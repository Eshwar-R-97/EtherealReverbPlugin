#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"

TEST_CASE ("CircularBuffer - fresh buffer reads as zero", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (16);

    for (int i = 1; i <= 16; ++i)
        REQUIRE (buf.read (i) == Catch::Approx (0.0f));
}

TEST_CASE ("CircularBuffer - read(1) returns the most recently written sample", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (16);

    buf.write (3.0f);
    REQUIRE (buf.read (1) == Catch::Approx (3.0f));

    buf.write (7.0f);
    REQUIRE (buf.read (1) == Catch::Approx (7.0f));
    REQUIRE (buf.read (2) == Catch::Approx (3.0f));
}

TEST_CASE ("CircularBuffer - read(N) returns sample written exactly N writes ago", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (64);

    // Write a known impulse then silence
    buf.write (1.0f);
    for (int i = 0; i < 9; ++i)
        buf.write (0.0f);

    // 10 writes have happened — the impulse is 10 positions back
    REQUIRE (buf.read (10) == Catch::Approx (1.0f));

    // Everything adjacent should be silent
    REQUIRE (buf.read (9)  == Catch::Approx (0.0f));
    REQUIRE (buf.read (11) == Catch::Approx (0.0f));
}

TEST_CASE ("CircularBuffer - write head wraps around correctly", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (4);

    buf.write (1.0f);
    buf.write (2.0f);
    buf.write (3.0f);
    buf.write (4.0f);
    buf.write (5.0f); // overwrites position 0, writeHead wraps

    REQUIRE (buf.read (1) == Catch::Approx (5.0f));
    REQUIRE (buf.read (2) == Catch::Approx (4.0f));
    REQUIRE (buf.read (3) == Catch::Approx (3.0f));
    REQUIRE (buf.read (4) == Catch::Approx (2.0f));
    // position 0 (value 1.0) has been overwritten
}

TEST_CASE ("CircularBuffer - reset zeros all data and resets write head", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (8);

    buf.write (1.0f);
    buf.write (2.0f);
    buf.write (3.0f);
    buf.reset();

    for (int i = 1; i <= 8; ++i)
        REQUIRE (buf.read (i) == Catch::Approx (0.0f));
}

TEST_CASE ("CircularBuffer - readInterpolated at integer offset matches read", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (16);

    buf.write (3.0f);
    buf.write (7.0f);
    buf.write (2.0f);

    REQUIRE (buf.readInterpolated (1.0f) == Catch::Approx (buf.read (1)));
    REQUIRE (buf.readInterpolated (2.0f) == Catch::Approx (buf.read (2)));
    REQUIRE (buf.readInterpolated (3.0f) == Catch::Approx (buf.read (3)));
}

TEST_CASE ("CircularBuffer - readInterpolated at 0.5 returns midpoint", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (16);

    buf.write (0.0f);
    buf.write (1.0f);

    // read(1)=1.0, read(2)=0.0 — midpoint at frac=0.5 is 0.5
    REQUIRE (buf.readInterpolated (1.5f) == Catch::Approx (0.5f));
}

TEST_CASE ("CircularBuffer - readInterpolated interpolates linearly", "[CircularBuffer]")
{
    CircularBuffer buf;
    buf.allocate (16);

    buf.write (0.0f);
    buf.write (4.0f);

    // read(1)=4.0, read(2)=0.0
    // at frac=0.25: 4.0 + 0.25 * (0.0 - 4.0) = 3.0
    REQUIRE (buf.readInterpolated (1.25f) == Catch::Approx (3.0f));
    // at frac=0.75: 4.0 + 0.75 * (0.0 - 4.0) = 1.0
    REQUIRE (buf.readInterpolated (1.75f) == Catch::Approx (1.0f));
}
