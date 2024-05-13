uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    vec4 newbase = vec4(0.5*pixel.r, 0.0, pixel.b, pixel.x);
    vec4 newpixel = newbase + vec4(sin(3.14*pixel.r), pixel.g, pixel.y, pixel.y);
    // I have no idea what pixel.x/y actually are (not coordinates)
    // newpixel = vec4(pixel.x, 0, pixel.y, pixel.x+pixel.y); // greyscale
    if (newpixel.a < 0.2) {
        newpixel.a = 0.0;
    }
    gl_FragColor = newpixel;
}
