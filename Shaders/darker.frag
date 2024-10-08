uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    pixel = vec4(1.f-pixel.r, 1.f-pixel.g, 1.f-pixel.b, pixel.a);
    gl_FragColor = cos(3.14 * pixel);
    gl_FragColor.a = pixel.a;
}
