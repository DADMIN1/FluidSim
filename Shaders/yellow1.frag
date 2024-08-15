// yellow-pinkish color scheme. possibly useful for cherry_blossoms
uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    pixel = vec4(0.5f*pixel.r, 0.5f*pixel.g, 0.5f*pixel.b, pixel.a);
    gl_FragColor = cos(3.14 * pixel);
    gl_FragColor.a = pixel.a;
}
