#pragma once

#include "Common.hpp"
#include "BlockDef.hpp"

struct Recipe {
  Slot input[9];
  Slot output;
};

constexpr Recipe CraftingList[] = {
    // Planks: Wood -> 4x Planks
    {{{1, (short)BlockIDDef::Wood, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {4, (short)BlockIDDef::Planks, false}},

    // Crafting Table: 4x Planks -> CraftingTable
    {{{1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {0, 0, false},
      {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {0, 0, false},
      {0, 0, false},                      {0, 0, false},                      {0, 0, false}},
     {1, (short)BlockIDDef::CraftingTable, false}},

    // Furnace: 8x Cobblestone -> Furnace
    {{{1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false},
      {1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false},                           {1, (short)BlockIDDef::Cobblestone, false},
      {1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}},
     {1, (short)BlockIDDef::Furnace, false}},

    // Chest: 8x Planks -> Chest
    {{{1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false},
      {1, (short)BlockIDDef::Planks, false}, {0, 0, false},                      {1, (short)BlockIDDef::Planks, false},
      {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}},
     {1, (short)BlockIDDef::Chest, false}},

    // Stick: 2x Planks -> 4x Stick
    {{{1, (short)BlockIDDef::Planks, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Planks, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false},                      {0, 0, false}, {0, 0, false}},
     {4, (short)BlockIDDef::Stick, false}},

    // DiamondIngot Block: 9x DiamondIngot -> DiamondBlock
    {{{1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false},
      {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false},
      {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}},
     {1, (short)BlockIDDef::DiamondBlock, false}},

    // DiamondIngot Uncraft: DiamondBlock -> 9x DiamondIngot
    {{{1, (short)BlockIDDef::DiamondBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::DiamondIngot, false}},

    // Gold Block: 9x GoldIngot -> GoldBlock
    {{{1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false},
      {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false},
      {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}},
     {1, (short)BlockIDDef::GoldBlock, false}},

    // Gold Uncraft: GoldBlock -> 9x GoldIngot
    {{{1, (short)BlockIDDef::GoldBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::GoldIngot, false}},

    // Iron Block: 9x IronIngot -> IronBlock
    {{{1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false},
      {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false},
      {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}},
     {1, (short)BlockIDDef::IronBlock, false}},

    // Iron Uncraft: IronBlock -> 9x IronIngot
    {{{1, (short)BlockIDDef::IronBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::IronIngot, false}},

    // Coal Block: 9x Coal -> CoalBlock
    {{{1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false},
      {1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false},
      {1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false}, {1, (short)BlockIDDef::Coal, false}},
     {1, (short)BlockIDDef::CoalBlock, false}},

    // Coal Uncraft: CoalBlock -> 9x Coal
    {{{1, (short)BlockIDDef::CoalBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::Coal, false}},

    // Emerald Block: 9x Emerald -> EmeraldBlock
    {{{1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false},
      {1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false},
      {1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false}, {1, (short)BlockIDDef::Emerald, false}},
     {1, (short)BlockIDDef::EmeraldBlock, false}},

    // Emerald Uncraft: EmeraldBlock -> 9x Emerald
    {{{1, (short)BlockIDDef::EmeraldBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::Emerald, false}},

    // Lapis Block: 9x LapisLazuli -> LapisBlock
    {{{1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false},
      {1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false},
      {1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false}, {1, (short)BlockIDDef::LapisLazuli, false}},
     {1, (short)BlockIDDef::LapisBlock, false}},

    // Lapis Uncraft: LapisBlock -> 9x LapisLazuli
    {{{1, (short)BlockIDDef::LapisBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::LapisLazuli, false}},

    // Raw Copper Block: 9x RawCopper -> RawCopperBlock
    {{{1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false},
      {1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false},
      {1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false}, {1, (short)BlockIDDef::RawCopper, false}},
     {1, (short)BlockIDDef::RawCopperBlock, false}},

    // Raw Copper Uncraft: RawCopperBlock -> 9x RawCopper
    {{{1, (short)BlockIDDef::RawCopperBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::RawCopper, false}},

    // Raw Gold Block: 9x RawGold -> RawGoldBlock
    {{{1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false},
      {1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false},
      {1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false}, {1, (short)BlockIDDef::RawGold, false}},
     {1, (short)BlockIDDef::RawGoldBlock, false}},

    // Raw Gold Uncraft: RawGoldBlock -> 9x RawGold
    {{{1, (short)BlockIDDef::RawGoldBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::RawGold, false}},

    // Raw Iron Block: 9x RawIron -> RawIronBlock
    {{{1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false},
      {1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false},
      {1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false}, {1, (short)BlockIDDef::RawIron, false}},
     {1, (short)BlockIDDef::RawIronBlock, false}},

    // Raw Iron Uncraft: RawIronBlock -> 9x RawIron
    {{{1, (short)BlockIDDef::RawIronBlock, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false},
      {0, 0, false}, {0, 0, false}, {0, 0, false}, {0, 0, false}},
     {9, (short)BlockIDDef::RawIron, false}},

    // Stone Bricks: 4x Stone -> 4x StoneBricks
    {{{1, (short)BlockIDDef::Stone, false}, {1, (short)BlockIDDef::Stone, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stone, false}, {1, (short)BlockIDDef::Stone, false}, {0, 0, false},
      {0, 0, false},                     {0, 0, false},                     {0, 0, false}},
     {4, (short)BlockIDDef::StoneBricks, false}},

    // Amethyst Block: 4x AmethystShard -> AmethystBlock
    {{{1, (short)BlockIDDef::AmethystShard, false}, {1, (short)BlockIDDef::AmethystShard, false}, {0, 0, false},
      {1, (short)BlockIDDef::AmethystShard, false}, {1, (short)BlockIDDef::AmethystShard, false}, {0, 0, false},
      {0, 0, false},                             {0, 0, false},                             {0, 0, false}},
     {1, (short)BlockIDDef::AmethystBlock, false}},

    // Wooden Sword: 2x Planks + 1x Stick -> WoodenSword
    {{{1, (short)BlockIDDef::Planks, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Planks, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::WoodenSword, false}},

    // Wooden Shovel: 1x Planks + 2x Stick -> WoodenShovel
    {{{1, (short)BlockIDDef::Planks, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::WoodenShovel, false}},

    // Wooden Pickaxe: 3x Planks + 2x Stick -> WoodenPickaxe
    {{{1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false},
      {0, 0, false},                      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false},
      {0, 0, false},                      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}},
     {1, (short)BlockIDDef::WoodenPickaxe, false}},

    // Wooden Axe: 2x Planks top-left + 2x Stick -> WoodenAxe
    {{{1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {0, 0, false},
      {1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Stick,  false}, {0, 0, false},
      {0, 0, false},                      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}},
     {1, (short)BlockIDDef::WoodenAxe, false}},

    // Wooden Hoe: 2x Planks + 2x Stick -> WoodenHoe
    {{{1, (short)BlockIDDef::Planks, false}, {1, (short)BlockIDDef::Planks, false}, {0, 0, false},
      {0, 0, false},                      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false},
      {0, 0, false},                      {1, (short)BlockIDDef::Stick,  false}, {0, 0, false}},
     {1, (short)BlockIDDef::WoodenHoe, false}},

    // Stone Sword: 2x Cobblestone + 1x Stick -> StoneSword
    {{{1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::StoneSword, false}},

    // Stone Shovel: 1x Cobblestone + 2x Stick -> StoneShovel
    {{{1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::StoneShovel, false}},

    // Stone Pickaxe: 3x Cobblestone + 2x Stick -> StonePickaxe
    {{{1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false},
      {0, 0, false},                           {1, (short)BlockIDDef::Stick,        false}, {0, 0, false},
      {0, 0, false},                           {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}},
     {1, (short)BlockIDDef::StonePickaxe, false}},

    // Stone Axe: 2x Cobblestone top-left + 2x Stick -> StoneAxe
    {{{1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false},
      {1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Stick,        false}, {0, 0, false},
      {0, 0, false},                           {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}},
     {1, (short)BlockIDDef::StoneAxe, false}},

    // Stone Hoe: 2x Cobblestone + 2x Stick -> StoneHoe
    {{{1, (short)BlockIDDef::Cobblestone, false}, {1, (short)BlockIDDef::Cobblestone, false}, {0, 0, false},
      {0, 0, false},                           {1, (short)BlockIDDef::Stick,        false}, {0, 0, false},
      {0, 0, false},                           {1, (short)BlockIDDef::Stick,        false}, {0, 0, false}},
     {1, (short)BlockIDDef::StoneHoe, false}},

    // DiamondIngot Sword: 2x DiamondIngot + 1x Stick -> DiamondSword
    {{{1, (short)BlockIDDef::DiamondIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::DiamondIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,   false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::DiamondSword, false}},

    // DiamondIngot Shovel: 1x DiamondIngot + 2x Stick -> DiamondShovel
    {{{1, (short)BlockIDDef::DiamondIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,   false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,   false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::DiamondShovel, false}},

    // DiamondIngot Axe: 2x DiamondIngot top-left + 2x Stick -> DiamondAxe
    {{{1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}, {0, 0, false},
      {1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::Stick,   false}, {0, 0, false},
      {0, 0, false},                       {1, (short)BlockIDDef::Stick,   false}, {0, 0, false}},
     {1, (short)BlockIDDef::DiamondAxe, false}},

    // DiamondIngot Hoe: 2x DiamondIngot + 2x Stick -> DiamondHoe
    {{{1, (short)BlockIDDef::DiamondIngot, false}, {1, (short)BlockIDDef::DiamondIngot, false}, {0, 0, false},
      {0, 0, false},                       {1, (short)BlockIDDef::Stick,   false}, {0, 0, false},
      {0, 0, false},                       {1, (short)BlockIDDef::Stick,   false}, {0, 0, false}},
     {1, (short)BlockIDDef::DiamondHoe, false}},

    // Iron Sword: 2x IronIngot + 1x Stick -> IronSword
    {{{1, (short)BlockIDDef::IronIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::IronIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::IronSword, false}},

    // Iron Shovel: 1x IronIngot + 2x Stick -> IronShovel
    {{{1, (short)BlockIDDef::IronIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::IronShovel, false}},

    // Iron Pickaxe: 3x IronIngot + 2x Stick -> IronPickaxe
    {{{1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::IronPickaxe, false}},

    // Iron Axe: 2x IronIngot top-left + 2x Stick -> IronAxe
    {{{1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {0, 0, false},
      {1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::IronAxe, false}},

    // Iron Hoe: 2x IronIngot + 2x Stick -> IronHoe
    {{{1, (short)BlockIDDef::IronIngot, false}, {1, (short)BlockIDDef::IronIngot, false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::IronHoe, false}},

    // Golden Sword: 2x GoldIngot + 1x Stick -> GoldenSword
    {{{1, (short)BlockIDDef::GoldIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::GoldIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::GoldenSword, false}},

    // Golden Shovel: 1x GoldIngot + 2x Stick -> GoldenShovel
    {{{1, (short)BlockIDDef::GoldIngot, false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::GoldenShovel, false}},

    // Golden Pickaxe: 3x GoldIngot + 2x Stick -> GoldenPickaxe
    {{{1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::GoldenPickaxe, false}},

    // Golden Axe: 2x GoldIngot top-left + 2x Stick -> GoldenAxe
    {{{1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {0, 0, false},
      {1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::GoldenAxe, false}},

    // Golden Hoe: 2x GoldIngot + 2x Stick -> GoldenHoe
    {{{1, (short)BlockIDDef::GoldIngot, false}, {1, (short)BlockIDDef::GoldIngot, false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false},
      {0, 0, false},                         {1, (short)BlockIDDef::Stick,     false}, {0, 0, false}},
     {1, (short)BlockIDDef::GoldenHoe, false}},

     // sticks and coal -> Torch
    {{{1, (short)BlockIDDef::Charcoal, false}, {1, 0, false}, {0, 0, false},
      {1, (short)BlockIDDef::Stick, false}, {1, 0, false}, {0, 0, false},
      {0, 0, false}, {1, 0, false}, {0, 0, false}},
     {1, (short)BlockIDDef::Glowstone, false}},
     
};

constexpr Recipe SmeltingList[] = {
  // Wood to Charcoal
  {{1, (short)BlockIDDef::Wood, false},
    {4, (short)BlockIDDef::Charcoal, false}},
  // Raw iron to cooked iron
  {{1, (short)BlockIDDef::RawIron, false},
    {1, (short)BlockIDDef::IronIngot, false}},
};

constexpr unsigned int CraftingListAmount =
    sizeof(CraftingList) / sizeof(CraftingList[0]);


constexpr unsigned int SmeltingListAmount =
    sizeof(SmeltingList) / sizeof(SmeltingList[0]);