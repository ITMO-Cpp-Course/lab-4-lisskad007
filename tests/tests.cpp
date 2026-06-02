#include <catch2/catch_all.hpp>

#include "file_handle.hpp"
#include "resource_error.hpp"
#include "resource_manager.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace lab4::resource;

struct TempFile
{
    fs::path path;

    explicit TempFile(const std::string& content = "")
        : path(fs::temp_directory_path() / ("lab4_test_" + std::to_string(counter++)))
    {
        std::ofstream out(path);
        out << content;
    }

    ~TempFile()
    {
        std::error_code ec;
        fs::remove(path, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

  private:
    static inline int counter = 0;
};

// ─── Тесты ResourceError ─────────────────────────────────────────────────────

TEST_CASE("ResourceError", "[ResourceError]")
{
    SECTION("is a std::runtime_error")
    {
        CHECK_THROWS_AS(throw ResourceError("boom"), std::runtime_error);
    }

    SECTION("carries the message")
    {
        try
        {
            throw ResourceError("something went wrong");
        }
        catch (const ResourceError& ex)
        {
            CHECK(std::string(ex.what()).find("something went wrong") != std::string::npos);
        }
    }
}

// ─── Тесты FileHandle: Конструкторы
// ───────────────────────────────────────────

TEST_CASE("FileHandle constructors", "[FileHandle]")
{
    SECTION("default-constructed FileHandle is not open")
    {
        FileHandle h;
        CHECK_FALSE(h.is_open());
        CHECK(h.get_filename().empty());
    }

    SECTION("opening an existing file succeeds")
    {
        TempFile tmp("hello");
        FileHandle h(tmp.path.string(), "r");
        CHECK(h.is_open());
        CHECK(h.get_filename() == tmp.path.string());
    }

    SECTION("opening a non-existent file throws ResourceError")
    {
        CHECK_THROWS_AS(FileHandle("/no/such/path/file.txt", "r"), ResourceError);
    }
}

// ─── Тесты FileHandle: open / close ──────────────────────────────────────────

TEST_CASE("FileHandle open/close operations", "[FileHandle]")
{
    SECTION("closing an open handle makes it not-open")
    {
        TempFile tmp;
        FileHandle h;
        h.open(tmp.path.string(), "r");
        REQUIRE(h.is_open());
        h.close();
        CHECK_FALSE(h.is_open());
    }

    SECTION("closing a not-open handle is a no-op")
    {
        FileHandle h;
        CHECK_NOTHROW(h.close());
        CHECK_FALSE(h.is_open());
    }

    SECTION("can reopen after close")
    {
        TempFile tmp("content");
        FileHandle h;
        h.open(tmp.path.string(), "r");
        REQUIRE(h.is_open());
        h.close();
        CHECK_FALSE(h.is_open());

        h.open(tmp.path.string(), "r");
        CHECK(h.is_open());
    }
}

// ─── Тесты FileHandle: чтение ───────────────────────────────────────────────

TEST_CASE("FileHandle read operations", "[FileHandle]")
{
    SECTION("read_line returns the first line")
    {
        TempFile tmp("hello world\nsecond line");
        FileHandle h(tmp.path.string(), "r");
        CHECK(h.read_line() == "hello world\n");
    }

    SECTION("read_line on an empty file returns empty string")
    {
        TempFile tmp("");
        FileHandle h(tmp.path.string(), "r");
        CHECK(h.read_line().empty());
    }

    SECTION("read_line on a closed handle throws")
    {
        FileHandle h;
        CHECK_THROWS_AS(h.read_line(), ResourceError);
    }

    SECTION("successive read_line calls read multiple lines")
    {
        TempFile tmp("line1\nline2\nline3");
        FileHandle h(tmp.path.string(), "r");
        CHECK(h.read_line() == "line1\n");
        CHECK(h.read_line() == "line2\n");
        CHECK(h.read_line() == "line3");
    }
}

// ─── Тесты FileHandle: режимы открытия ───────────────────────────────────────

TEST_CASE("FileHandle open modes", "[FileHandle][mode]")
{
    TempFile tmp("test content");

    SECTION("read mode 'r'")
    {
        FileHandle h(tmp.path.string(), "r");
        CHECK(h.is_open());
        CHECK(h.read_line() == "test content");
    }

    SECTION("write mode 'w' creates/truncates file")
    {
        FileHandle h(tmp.path.string(), "w");
        CHECK(h.is_open());
        // В режиме записи нельзя читать
        CHECK_THROWS_AS(h.read_line(), ResourceError);
    }

    SECTION("append mode 'a'")
    {
        FileHandle h(tmp.path.string(), "a");
        CHECK(h.is_open());
    }
}

// ─── Тесты move-семантики
// ─────────────────────────────────────────────────────

TEST_CASE("FileHandle move semantics", "[FileHandle][ownership]")
{
    TempFile tmp("data");
    FileHandle a(tmp.path.string(), "r");
    REQUIRE(a.is_open());

    FileHandle b(std::move(a));

    CHECK_FALSE(a.is_open()); // После перемещения a больше не владеет ресурсом
    CHECK(b.is_open());
    CHECK(b.read_line() == "data");
}

// ─── Тесты ResourceManager
// ────────────────────────────────────────────────────

TEST_CASE("ResourceManager get_resource operations", "[ResourceManager]")
{
    TempFile tmp("content");

    SECTION("get_resource returns a non-null handle")
    {
        ResourceManager mgr;
        auto h = mgr.get_resource(tmp.path.string());
        CHECK(h != nullptr);
        CHECK(h->is_open());
    }

    SECTION("returns the same handle for the same path")
    {
        ResourceManager mgr;
        auto h1 = mgr.get_resource(tmp.path.string());
        auto h2 = mgr.get_resource(tmp.path.string());
        CHECK(h1.get() == h2.get());
    }

    SECTION("two handles from the same manager share the resource")
    {
        // Для FILE* невозможно проверить через запись, так как режим только для
        // чтения Создадим файл и проверим, что оба хендла указывают на один и тот
        // же FILE*
        TempFile tmp("original");
        ResourceManager mgr;
        auto h1 = mgr.get_resource(tmp.path.string());
        auto h2 = mgr.get_resource(tmp.path.string());
        CHECK(h1.get() == h2.get());
        CHECK(h1->get() == h2->get()); // Один и тот же FILE*
    }

    SECTION("different modes give different handles")
    {
        TempFile tmp("hello");
        ResourceManager mgr;
        auto h1 = mgr.get_resource(tmp.path.string(), "r");
        auto h2 = mgr.get_resource(tmp.path.string(), "w");
        // В вашей реализации режим влияет на ключ, поэтому хендлы разные
        CHECK(h1.get() != h2.get());
    }
}

TEST_CASE("ResourceManager cache behavior", "[ResourceManager][cache]")
{
    SECTION("cache entry expires after all shared_ptrs are released")
    {
        TempFile tmp;
        ResourceManager mgr;
        {
            auto h = mgr.get_resource(tmp.path.string());
            CHECK(mgr.cache_size() == 1);
        }
        // weak_ptr истек, но cleanup еще не вызывали
        mgr.cleanup();
        CHECK(mgr.cache_size() == 0);
    }

    SECTION("cleanup() removes expired cache entries")
    {
        TempFile tmp;
        ResourceManager mgr;
        {
            auto h = mgr.get_resource(tmp.path.string());
            CHECK(mgr.cache_size() == 1);
        }
        mgr.cleanup();
        CHECK(mgr.cache_size() == 0);
    }

    SECTION("after cleanup, reopening gives a fresh handle")
    {
        TempFile tmp("initial");
        ResourceManager mgr;
        std::shared_ptr<FileHandle> first;
        {
            auto h = mgr.get_resource(tmp.path.string());
            first = h;
        }
        mgr.cleanup();
        auto second = mgr.get_resource(tmp.path.string());
        CHECK(second.get() != first.get());
        CHECK(second->is_open());
    }

    SECTION("multiple files are cached independently")
    {
        TempFile t1("aaa");
        TempFile t2("bbb");
        ResourceManager mgr;
        auto h1 = mgr.get_resource(t1.path.string());
        auto h2 = mgr.get_resource(t2.path.string());
        CHECK(h1.get() != h2.get());
        CHECK(mgr.cache_size() == 2);
    }
}

// ─── Тесты release метода
// ─────────────────────────────────────────────────────

TEST_CASE("ResourceManager release", "[ResourceManager]")
{
    SECTION("release removes entry from cache")
    {
        TempFile tmp;
        ResourceManager mgr;
        auto h = mgr.get_resource(tmp.path.string());
        CHECK(mgr.cache_size() == 1);

        mgr.release(tmp.path.string());
        mgr.cleanup();
        CHECK(mgr.cache_size() == 0);
    }
}