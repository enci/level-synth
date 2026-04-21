"""
Icon generation using Skia
"""
import skia

# Gradient colors
COLOR_START = skia.Color4f(0.2, 0.4, 0.8, 1.0)  # Blue
COLOR_END = skia.Color4f(0.9, 0.3, 0.5, 1.0)    # Pink

# Base rounded rect parameters
BASE_MARGIN = 0.06       # fraction of size
BASE_ROUNDING = 0.18     # fraction of size

# LS letter layout in design-space units (6 wide × 5 tall)
LS_STROKE_WIDTH = 1.2   # design-space units
LS_SCALE = 0.55          # fraction of size the letters occupy


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
    Create icon: gradient rounded-rect base with white stroked LS letters.
    """
    surface = skia.Surface(size, size)
    canvas = surface.getCanvas()
    canvas.clear(skia.ColorTRANSPARENT)

    # --- Base: diagonal-gradient rounded rect ---
    margin = size * BASE_MARGIN
    rounding = size * BASE_ROUNDING
    base_rect = skia.Rect.MakeLTRB(margin, margin, size - margin, size - margin)
    gradient = skia.GradientShader.MakeLinear(
        points=[(margin, margin), (size - margin, size - margin)],
        colors=[COLOR_START.toColor(), COLOR_END.toColor()],
    )
    canvas.drawRoundRect(base_rect, rounding, rounding,
                         skia.Paint(AntiAlias=True, Shader=gradient))

    # --- LS letters: stroked white paths ---
    # Design space: 6 wide × 5 tall  (L: x 0–2, S: x 3–6)
    design_w, design_h = 6.0, 5.0
    target = size * LS_SCALE
    scale = min(target / design_w, target / design_h)
    offset_x = (size - design_w * scale) / 2
    offset_y = (size - design_h * scale) / 2

    canvas.save()
    canvas.translate(offset_x, offset_y)
    canvas.scale(scale, scale)

    # L: vertical stroke + bottom horizontal (extends to meet S)
    l_path = skia.Path()
    l_path.moveTo(0, 0)
    l_path.lineTo(0, 5)
    l_path.lineTo(3, 5)

    # S: top bar → upper-left down → mid bar → lower-right down → bottom bar
    s_path = skia.Path()
    s_path.moveTo(6, 0)
    s_path.lineTo(3, 0)
    s_path.lineTo(3, 2.5)
    s_path.lineTo(6, 2.5)
    s_path.lineTo(6, 5)
    s_path.lineTo(3, 5)

    stroke_paint = skia.Paint(
        AntiAlias=True,
        Color4f=skia.Color4f(1.0, 1.0, 1.0, 1.0),
        Style=skia.Paint.kStroke_Style,
        StrokeWidth=LS_STROKE_WIDTH,
    )
    stroke_paint.setStrokeCap(skia.Paint.kRound_Cap)
    stroke_paint.setStrokeJoin(skia.Paint.kRound_Join)

    canvas.drawPath(l_path, stroke_paint)
    canvas.drawPath(s_path, stroke_paint)
    canvas.restore()

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
