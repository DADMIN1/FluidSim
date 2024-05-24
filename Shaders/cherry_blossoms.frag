uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    float oldred = pixel.r;
    pixel.r = pixel.b;
    
    vec4 newpixel = 0.75*pixel + vec4(tan(total_channelvals*pixel.r), 0.1*sin(total_channelvals*pixel.g), tan(total_channelvals*pixel.b), pixel.a);
    gl_FragColor = newpixel;
}

// cool alt-turbulence?
void altmain() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    vec4 newbase = vec4(tan(total_channelvals*pixel.r), sin(total_channelvals*pixel.g), tan(total_channelvals*pixel.b), pixel.x);
    vec4 newpixel = newbase + vec4(sin(3.14*pixel.r), 0.0, pixel.y, pixel.y);
    gl_FragColor = newpixel * vec4(1.57*pixel.r, -0.2*pixel.g, pixel.b, pixel.a);
}

// very slight modification to turbulence
void altmaintwo() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    pixel.r = pixel.b;
    pixel.a = 1.1 - (total_channelvals / 3.0);
    
    vec4 newbase = vec4(0.5*pixel.r, 0.0, pixel.b, pixel.x);
    vec4 newpixel = newbase + vec4(sin(3.14*pixel.r), pixel.g, pixel.y, pixel.y);
    // I have no idea what pixel.x/y actually are (not coordinates)
    // newpixel = vec4(pixel.x, 0, pixel.y, pixel.x+pixel.y); // greyscale
    gl_FragColor = newpixel;
}
