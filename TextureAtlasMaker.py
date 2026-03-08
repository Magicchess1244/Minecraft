"""
Texture Atlas Appender
======================
- Takes an existing texture atlas (PNG)
- Reads all PNGs from a specified folder
- Scales them down 10x (160x160 → 16x16)
- Appends them to the atlas AFTER the existing textures
- Existing textures are 16x16; folder textures are 160x160 (scaled to 16x16)
"""

from PIL import Image
import os
import sys


def get_atlas_columns(atlas_width: int, tile_size: int) -> int:
    cols = atlas_width // tile_size
    if cols == 0:
        raise ValueError(f"Atlas width {atlas_width} is smaller than tile size {tile_size}.")
    return cols


def load_and_scale_pngs(folder: str, target_size: int = 16) -> list[Image.Image]:
    images = []
    supported = (".png",)
    entries = sorted(
        [f for f in os.listdir(folder) if f.lower().endswith(supported)]
    )
    if not entries:
        print(f"  [!] No PNG files found in '{folder}'.")
        return images

    for filename in entries:
        path = os.path.join(folder, filename)
        img = Image.open(path).convert("RGBA")
        if img.width != 160 or img.height != 160:
            print(f"  [!] Warning: '{filename}' is {img.width}x{img.height}, expected 160x160. Scaling anyway.")
        scaled = img.resize((target_size, target_size), Image.LANCZOS)
        images.append((filename, scaled))
        print(f"  ✓ Loaded & scaled: {filename}")
    return images


def append_textures_to_atlas(
    atlas_path: str,
    folder_path: str,
    existing_texture_count: int,
    tile_size: int = 16,
) -> None:
    # Load atlas
    atlas = Image.open(atlas_path).convert("RGBA")
    atlas_w, atlas_h = atlas.size
    cols = get_atlas_columns(atlas_w, tile_size)

    print(f"\n  Atlas size     : {atlas_w}x{atlas_h}")
    print(f"  Tile size      : {tile_size}x{tile_size}")
    print(f"  Columns        : {cols}")
    print(f"  Existing tiles : {existing_texture_count}")

    # Load new images from folder
    print(f"\nLoading images from '{folder_path}' ...")
    new_images = load_and_scale_pngs(folder_path, target_size=tile_size)

    if not new_images:
        print("Nothing to add. Exiting.")
        return

    # Figure out where new textures start (slot index)
    start_slot = existing_texture_count
    total_slots_needed = start_slot + len(new_images)

    # Calculate required atlas height
    rows_needed = (total_slots_needed + cols - 1) // cols
    new_atlas_h = rows_needed * tile_size

    # Expand canvas if necessary
    if new_atlas_h > atlas_h:
        expanded = Image.new("RGBA", (atlas_w, new_atlas_h), (0, 0, 0, 0))
        expanded.paste(atlas, (0, 0))
        atlas = expanded
        print(f"\n  Atlas expanded to {atlas_w}x{new_atlas_h}")

    # Paste new textures
    print(f"\nPasting {len(new_images)} texture(s) starting at slot {start_slot} ...")
    for i, (filename, img) in enumerate(new_images):
        slot = start_slot + i
        row = slot // cols
        col = slot % cols
        x = col * tile_size
        y = row * tile_size
        atlas.paste(img, (x, y))
        print(f"  ✓ '{filename}' → slot {slot} (col={col}, row={row}) @ ({x},{y})")

    # Save result
    out_path = atlas_path.replace(".png", "_updated.png")
    atlas.save(out_path)
    print(f"\n✅ Done! Updated atlas saved to: {out_path}")


def main():
    print("=== Texture Atlas Appender ===\n")

    # Atlas path
    atlas_path = input("Path to your texture atlas PNG: ").strip().strip('"')
    if not os.path.isfile(atlas_path):
        print(f"Error: File not found: {atlas_path}")
        sys.exit(1)

    # Folder path
    folder_path = input("Path to folder containing new 160x160 PNGs: ").strip().strip('"')
    if not os.path.isdir(folder_path):
        print(f"Error: Directory not found: {folder_path}")
        sys.exit(1)

    # Existing texture count
    while True:
        try:
            existing_count = int(input("How many textures are already in your atlas? ").strip())
            if existing_count < 0:
                raise ValueError
            break
        except ValueError:
            print("Please enter a valid non-negative integer.")

    append_textures_to_atlas(
        atlas_path=atlas_path,
        folder_path=folder_path,
        existing_texture_count=existing_count,
        tile_size=16,
    )


if __name__ == "__main__":
    main()
