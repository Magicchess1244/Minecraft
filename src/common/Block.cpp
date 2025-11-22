/*
#include <stdbool.h>
#include "../../include/common/Block.hpp"

bool block_opaque(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    switch (block)
    {
    case BLOCK_WATER:
        return false;
    }
    return true;
}

bool block_shadow(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    if (block_sprite(block))
    {
        return false;
    }
    switch (block)
    {
    case BLOCK_CLOUD:
        return false;
    }
    return true;
}

bool block_shadowed(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    switch (block)
    {
    case BLOCK_CLOUD:
        return false;
    }
    return true;
}

bool block_occluded(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    switch (block)
    {
    case BLOCK_CLOUD:
        return false;
    }
    return true;
}

bool block_solid(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    if (block_sprite(block))
    {
        return false;
    }
    switch (block)
    {
    case BLOCK_EMPTY:
    case BLOCK_WATER:
        return false;
    }
    return true;
}

bool block_sprite(
    const block_t block)
{
    assert(block < BLOCK_COUNT);
    switch (block)
    {
    case BLOCK_BUSH:
    case BLOCK_BLUEBELL:
    case BLOCK_DANDELION:
    case BLOCK_LAVENDER:
    case BLOCK_ROSE:
        return true;
    }
    return false;
}

const int blocks[][DIRECTION_3] =
{
    [BLOCK_BLUEBELL] =
    {
        [DIRECTION_E] = 13,
        [DIRECTION_W] = 13,
        [DIRECTION_N] = 13,
        [DIRECTION_S] = 13,
        [DIRECTION_U] = 13,
        [DIRECTION_D] = 13,
    },
    [BLOCK_LAVENDER] =
    {
        [DIRECTION_E] = 14,
        [DIRECTION_W] = 14,
        [DIRECTION_N] = 14,
        [DIRECTION_S] = 14,
        [DIRECTION_U] = 14,
        [DIRECTION_D] = 14,
    },
    [BLOCK_CLOUD] =
    {
        [DIRECTION_E] = 9,
        [DIRECTION_W] = 9,
        [DIRECTION_N] = 9,
        [DIRECTION_S] = 9,
        [DIRECTION_U] = 9,
        [DIRECTION_D] = 9,
    },
    [BLOCK_DANDELION] =
    {
        [DIRECTION_E] = 12,
        [DIRECTION_W] = 12,
        [DIRECTION_N] = 12,
        [DIRECTION_S] = 12,
        [DIRECTION_U] = 12,
        [DIRECTION_D] = 12,
    },
    [BLOCK_BUSH] =
    {
        [DIRECTION_E] = 15,
        [DIRECTION_W] = 15,
        [DIRECTION_N] = 15,
        [DIRECTION_S] = 15,
        [DIRECTION_U] = 15,
        [DIRECTION_D] = 15,
    },
    [BLOCK_DIRT] =
    {
        [DIRECTION_E] = 3,
        [DIRECTION_W] = 3,
        [DIRECTION_N] = 3,
        [DIRECTION_S] = 3,
        [DIRECTION_U] = 3,
        [DIRECTION_D] = 3,
    },
    [BLOCK_GRASS] =
    {
        [DIRECTION_E] = 2,
        [DIRECTION_W] = 2,
        [DIRECTION_N] = 2,
        [DIRECTION_S] = 2,
        [DIRECTION_U] = 1,
        [DIRECTION_D] = 3,
    },
    [BLOCK_LEAVES] =
    {
        [DIRECTION_E] = 10,
        [DIRECTION_W] = 10,
        [DIRECTION_N] = 10,
        [DIRECTION_S] = 10,
        [DIRECTION_U] = 10,
        [DIRECTION_D] = 10,
    },
    [BLOCK_LOG] =
    {
        [DIRECTION_E] = 8,
        [DIRECTION_W] = 8,
        [DIRECTION_N] = 8,
        [DIRECTION_S] = 8,
        [DIRECTION_U] = 7,
        [DIRECTION_D] = 7,
    },
    [BLOCK_ROSE] =
    {
        [DIRECTION_E] = 11,
        [DIRECTION_W] = 11,
        [DIRECTION_N] = 11,
        [DIRECTION_S] = 11,
        [DIRECTION_U] = 11,
        [DIRECTION_D] = 11,
    },
    [BLOCK_SAND] =
    {
        [DIRECTION_E] = 5,
        [DIRECTION_W] = 5,
        [DIRECTION_N] = 5,
        [DIRECTION_S] = 5,
        [DIRECTION_U] = 5,
        [DIRECTION_D] = 5,
    },
    [BLOCK_SNOW] =
    {
        [DIRECTION_E] = 6,
        [DIRECTION_W] = 6,
        [DIRECTION_N] = 6,
        [DIRECTION_S] = 6,
        [DIRECTION_U] = 6,
        [DIRECTION_D] = 6,
    },
    [BLOCK_STONE] =
    {
        [DIRECTION_E] = 4,
        [DIRECTION_W] = 4,
        [DIRECTION_N] = 4,
        [DIRECTION_S] = 4,
        [DIRECTION_U] = 4,
        [DIRECTION_D] = 4,
    },
    [BLOCK_WATER] =
    {
        [DIRECTION_E] = 16,
        [DIRECTION_W] = 16,
        [DIRECTION_N] = 16,
        [DIRECTION_S] = 16,
        [DIRECTION_U] = 16,
        [DIRECTION_D] = 16,
    },
};*/