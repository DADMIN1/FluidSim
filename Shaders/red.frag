uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    
    //gl_FragColor = vec4(cos(1.57*pixel.r), pixel.g, cos(3.14*pixel.b), pixel.a); // good alt
    gl_FragColor = vec4(cos(1.57*pixel.r), sin(pixel.g), cos(3.14*2*pixel.b), pixel.a);
}


void altmain() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    vec4 newbase = vec4(tan(total_channelvals), sin(total_channelvals*pixel.g), tan(total_channelvals/pixel.r), pixel.x);
    //vec4 newpixel = newbase + vec4(sin(3.14*pixel.r), 0.0, pixel.y, pixel.y);
    gl_FragColor = newbase + vec4(1.57*pixel.r, -0.2*pixel.g, pixel.b, pixel.a);
}
