#include <catch2/catch_all.hpp>

#include <resource_core/resource_core.hpp>

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

    SECTION("with path includes path in message")
    {
        try
        {
            throw ResourceError("cannot open", fs::path("/tmp/missing.txt"));
        }
        catch (const ResourceError& ex)
        {
            const std::string msg(ex.what());
            CHECK(msg.find("cannot open") != std::string::npos);
            CHECK(msg.find("missing.txt") != std::string::npos);
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
        CHECK_FALSE(h.isOpen());
        CHECK(h.path().empty());
    }

    SECTION("opening an existing file succeeds")
    {
        TempFile tmp("hello");
        FileHandle h(tmp.path);
        CHECK(h.isOpen());
        CHECK(h.path() == tmp.path);
    }

    SECTION("opening a non-existent file throws ResourceError")
    {
        CHECK_THROWS_AS(FileHandle(fs::path("/no/such/path/file.txt")), ResourceError);
    }
}

// ─── Тесты FileHandle: close / reset
// ──────────────────────────────────────────

TEST_CASE("FileHandle close operations", "[FileHandle]")
{
    SECTION("closing an open handle makes it not-open")
    {
        TempFile tmp;
        FileHandle h(tmp.path);
        REQUIRE(h.isOpen());
        h.close();
        CHECK_FALSE(h.isOpen());
    }

    SECTION("closing a not-open handle is a no-op")
    {
        FileHandle h;
        CHECK_NOTHROW(h.close());
        CHECK_FALSE(h.isOpen());
    }

    SECTION("reset() closes the file")
    {
        TempFile tmp;
        FileHandle h(tmp.path);
        REQUIRE(h.isOpen());
        h.reset();
        CHECK_FALSE(h.isOpen());
    }
}

// ─── Тесты FileHandle: Чтение и запись ───────────────────────────────────────

TEST_CASE("FileHandle read operations", "[FileHandle]")
{
    SECTION("readAll returns the file content")
    {
        TempFile tmp("hello world");
        FileHandle h(tmp.path);
        CHECK(h.readAll() == "hello world");
    }

    SECTION("readAll on an empty file returns empty string")
    {
        TempFile tmp;
        FileHandle h(tmp.path);
        CHECK(h.readAll().empty());
    }

    SECTION("readAll on a closed handle throws ResourceError")
    {
        FileHandle h;
        CHECK_THROWS_AS(h.readAll(), ResourceError);
    }

    SECTION("readAll does not change the observable stream position")
    {
        TempFile tmp("abcdef");
        FileHandle h(tmp.path);
        REQUIRE(h.readAll() == "abcdef");
        CHECK(h.readAll() == "abcdef"); // второй вызов должен вернуть те же данные
    }
}

TEST_CASE("FileHandle write operations", "[FileHandle]")
{
    SECTION("write replaces file content")
    {
        TempFile tmp("old content");
        FileHandle h(tmp.path);
        h.write("new content");
        CHECK(h.readAll() == "new content");
    }

    SECTION("append adds to existing content")
    {
        TempFile tmp("hello ");
        FileHandle h(tmp.path);
        h.append("world");
        CHECK(h.readAll() == "hello world");
    }
}

// ─── Тесты режимов открытия
// ───────────────────────────────────────────────────

TEST_CASE("FileHandle open modes", "[FileHandle][mode]")
{
    TempFile tmp("x");

    SECTION("in-only")
    {
        FileHandle h(tmp.path, std::ios::in);
        CHECK(h.isReadable());
        CHECK_FALSE(h.isWritable());
    }

    SECTION("out-only")
    {
        FileHandle h(tmp.path, std::ios::out);
        CHECK_FALSE(h.isReadable());
        CHECK(h.isWritable());
    }

    SECTION("in|out")
    {
        FileHandle h(tmp.path, std::ios::in | std::ios::out);
        CHECK(h.isReadable());
        CHECK(h.isWritable());
    }
}

// ─── Тесты move-семантики
// ─────────────────────────────────────────────────────

TEST_CASE("FileHandle move semantics", "[FileHandle][ownership]")
{
    TempFile tmp("data");
    FileHandle a(tmp.path);
    REQUIRE(a.isOpen());

    FileHandle b(std::move(a));

    CHECK_FALSE(a.isOpen()); // NOLINT(bugprone-use-after-move)
    CHECK(b.isOpen());
    CHECK(b.readAll() == "data");
}

// ─── Тесты ResourceManager
// ────────────────────────────────────────────────────

TEST_CASE("ResourceManager open operations", "[ResourceManager]")
{
    TempFile tmp;

    SECTION("open returns a non-null handle")
    {
        ResourceManager mgr;
        auto h = mgr.open(tmp.path);
        CHECK(h != nullptr);
        CHECK(h->isOpen());
    }

    SECTION("returns the same handle for the same path and mode")
    {
        ResourceManager mgr;
        auto h1 = mgr.open(tmp.path);
        auto h2 = mgr.open(tmp.path);
        CHECK(h1.get() == h2.get());
    }

    SECTION("two handles from the same manager share the resource")
    {
        TempFile tmp("original");
        ResourceManager mgr;
        auto h1 = mgr.open(tmp.path);
        auto h2 = mgr.open(tmp.path);
        h1->write("changed");
        CHECK(h2->readAll() == "changed");
    }

    SECTION("same path with different modes gives different handles")
    {
        TempFile tmp("hello");
        ResourceManager mgr;
        auto hr = mgr.open(tmp.path, std::ios::in);
        auto hw = mgr.open(tmp.path, std::ios::out);
        CHECK(hr.get() != hw.get());
    }

    SECTION("same path with same mode gives the same handle")
    {
        TempFile tmp;
        ResourceManager mgr;
        auto h1 = mgr.open(tmp.path, std::ios::in | std::ios::out);
        auto h2 = mgr.open(tmp.path, std::ios::in | std::ios::out);
        CHECK(h1.get() == h2.get());
    }
}

TEST_CASE("ResourceManager cache behavior", "[ResourceManager][cache]")
{
    SECTION("cache entry expires after all shared_ptrs are released")
    {
        TempFile tmp;
        ResourceManager mgr;
        {
            auto h = mgr.open(tmp.path);
            CHECK(mgr.liveCacheSize() == 1);
        }
        CHECK(mgr.liveCacheSize() == 0);
        CHECK(mgr.cacheSize() == 1);
    }

    SECTION("purge() removes expired cache entries")
    {
        TempFile tmp;
        ResourceManager mgr;
        {
            auto h = mgr.open(tmp.path);
        }
        mgr.purge();
        CHECK(mgr.cacheSize() == 0);
    }

    SECTION("after eviction, reopening gives a fresh handle")
    {
        TempFile tmp("initial");
        ResourceManager mgr;
        std::shared_ptr<FileHandle> first;
        {
            auto h = mgr.open(tmp.path);
            first = h;
        }
        first.reset();
        auto second = mgr.open(tmp.path);
        CHECK(second.get() != first.get());
        CHECK(second->isOpen());
    }

    SECTION("multiple files are cached independently")
    {
        TempFile t1("aaa");
        TempFile t2("bbb");
        ResourceManager mgr;
        auto h1 = mgr.open(t1.path);
        auto h2 = mgr.open(t2.path);
        CHECK(h1.get() != h2.get());
        CHECK(mgr.liveCacheSize() == 2);
    }

    SECTION("liveCacheSize counts only non-expired entries")
    {
        TempFile t1, t2, t3;
        ResourceManager mgr;
        auto h1 = mgr.open(t1.path);
        auto h2 = mgr.open(t2.path);
        {
            auto h3 = mgr.open(t3.path);
            CHECK(mgr.liveCacheSize() == 3);
        }
        CHECK(mgr.liveCacheSize() == 2);
        CHECK(mgr.cacheSize() == 3);
    }
}

// ─── Тесты ResourceKey
// ────────────────────────────────────────────────────────

TEST_CASE("ResourceKey equality and hash", "[ResourceKey]")
{
    TempFile tmp;
    const auto canonical = std::filesystem::weakly_canonical(tmp.path);

    SECTION("equality is consistent")
    {
        const ResourceKey k1{canonical, std::ios::in};
        const ResourceKey k2{canonical, std::ios::in};
        const ResourceKey k3{canonical, std::ios::out};

        CHECK(k1 == k2);
        CHECK_FALSE(k1 == k3);
    }

    SECTION("hash is consistent")
    {
        const ResourceKey k1{canonical, std::ios::in};
        const ResourceKey k2{canonical, std::ios::in};
        const ResourceKey k3{canonical, std::ios::out};

        const std::hash<ResourceKey> h;
        CHECK(h(k1) == h(k2));
        CHECK(h(k1) != h(k3));
    }
}