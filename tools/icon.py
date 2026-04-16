"""
Icon generation using Skia
"""
import skia

# Gradient colors
COLOR_START = skia.Color4f(0.2, 0.4, 0.8, 1.0)  # Blue
COLOR_END = skia.Color4f(0.9, 0.3, 0.5, 1.0)    # Pink

# Tile parameters (as fraction of cell size)
TILE_PADDING = 0.1
TILE_FROM_ROUNDING = 0.1
TILE_TO_ROUNDING = 0.1


def lerp_color(color1: skia.Color4f, color2: skia.Color4f, t: float) -> skia.Color4f:
    """
    Linearly interpolate between two colors.
    
    Args:
        color1: Start color
        color2: End color
        t: Interpolation factor (0.0 to 1.0)
    
    Returns:
        Interpolated color
    """
    return skia.Color4f(
        color1.fR + (color2.fR - color1.fR) * t,
        color1.fG + (color2.fG - color1.fG) * t,
        color1.fB + (color2.fB - color1.fB) * t,
        color1.fA + (color2.fA - color1.fA) * t,
    )


def create_test_icon(output_path: str, size: int = 256):
    """
    Create a 7x7 grid icon with gradient colors and "LS" letters using Skia.
    
    Args:
        output_path: Path where the icon PNG will be saved
        size: Icon size in pixels (default 256x256)
    """
    # Create a surface
    surface = skia.Surface(size, size)
    canvas = surface.getCanvas()
    
    # Clear background to white
    canvas.clear(skia.ColorWHITE)
    
    # Grid parameters
    grid_size = 7
    cell_size = size / grid_size
    padding = cell_size * TILE_PADDING
    from_rounding = cell_size * TILE_FROM_ROUNDING
    to_rounding = cell_size * TILE_TO_ROUNDING
    
    # Define which cells should be white to spell "LS"
    # Using (row, col) tuples - padding is row/col 0 and 6
    white_cells = {
        # L letter (column 1, rows 1-5, plus bottom row)
        (1, 1), (2, 1), (3, 1), (4, 1), (5, 1),
        (5, 2), (5, 3),
        # S letter (columns 3-5, rows 1-5)
        (1, 3), (1, 4), (1, 5),
        (2, 3),
        (3, 3), (3, 4), (3, 5),
        (4, 5),
        (5, 3), (5, 4), (5, 5),
    }
    
    # Draw grid with gradient
    for row in range(grid_size):
        for col in range(grid_size):
            # Check if this cell should be white
            if (row, col) in white_cells:
                color = skia.Color4f(1.0, 1.0, 1.0, 0.0)  # White
                rounding = 0  # No rounding for white cells
            else:
                # Calculate gradient position (diagonal gradient)
                t = (row + col) / (2 * (grid_size - 1))
                rounding = from_rounding + (to_rounding - from_rounding) * (1.0-t)
                # Get interpolated color
                color = lerp_color(COLOR_START, COLOR_END, t)
            
            # Create paint for this cell
            paint = skia.Paint(
                AntiAlias=True,
                Color4f=color,
                Style=skia.Paint.kFill_Style,
            )
            
            # Calculate cell position with padding
            x = col * cell_size + padding
            y = row * cell_size + padding
            w = cell_size - 2 * padding
            h = cell_size - 2 * padding
            
            # Draw rounded rectangle
            rect = skia.Rect(x, y, x + w, y + h)
            canvas.drawRoundRect(rect, rounding, rounding, paint)
    
    # Save the image
    image = surface.makeImageSnapshot()
    image.save(output_path, skia.kPNG)
    
    print(f"Icon saved to: {output_path}")


def main():
    """Test icon generation"""
    import os
    
    # Create output directory if it doesn't exist
    output_dir = os.path.join(os.path.dirname(__file__), '..', 'resources')
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate test icon
    output_path = os.path.join(output_dir, 'icon.png')
    create_test_icon(output_path, size=256)
    
    print("Test icon created successfully!")


if __name__ == "__main__":
    main()
