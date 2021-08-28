uniform vec2 origin;
uniform vec2 size;
uniform float opacity;

uniform sampler2D u_texture1;

void
mainImage(out vec4 fragColor,
          in vec2  fragCoord,
          in vec2  resolution,
          in vec2  uv)
{
  if (fragCoord.x >= origin.x && fragCoord.x <= origin.x + size.x &&
      fragCoord.y >= origin.y && fragCoord.y <= origin.y + size.y)
    fragColor = vec4(0, 0, 0, 0);
  else
    fragColor = opacity * GskTexture(u_texture1, uv);
}
