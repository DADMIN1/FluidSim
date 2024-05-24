// Only shows moving particles
uniform sampler2D texture;
uniform float threshold;
bool isDebug = false;


bool ExcludeBackground(vec4 pixel)
{
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    if (total_channelvals < 0.001) {
        if (isDebug)
        {
            gl_FragColor.a = 0.5;
            gl_FragColor.g = 1.0-threshold;
        }
        else
        {
            gl_FragColor.a = 0.0;
            gl_FragDepth = 0.0;
            discard;
        }
        return true;
    }
    return false;
}


void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    float total_channelvals = pixel.r + pixel.g + pixel.b;
    // the gradient mostly uses only two colors at any given point; 
    // therefore the max value is ~2.0 instead of 3.0
    float ratio = total_channelvals / 2.0;
    //gl_FragColor.a = ratio - threshold;
    //gl_FragColor.a = pixel.r + pixel.g;
    //gl_FragDepth = ratio;
    
    // exclude the background
    bool wasBackground = ExcludeBackground(pixel);
    if (wasBackground) return;
    
    float threshold_test = total_channelvals - threshold;
    gl_FragDepth -= threshold_test; // plus or minus both seem pretty good
    pixel.a -= threshold_test;
    if (threshold_test < 0.0) {
        if (isDebug) {
            gl_FragColor.r = 1.0;
            gl_FragColor.a = 0.5;
            return;
        }
        
        //discard;
        return;
    }
    
    vec4 newbase = vec4(0.5*pixel.r, 0.0, pixel.b, pixel.x);
    vec4 newpixel = newbase + vec4(sin(3.14*pixel.r), pixel.g, pixel.y, pixel.y);
    //vec4 newbase = vec4(0.25*(pixel.r+pixel.g), 0.0, sin(3.14*ratio), pixel.a);
    //vec4 newpixel = 0.5*newbase + 0.5*vec4(0.25*sin(3.14*pixel.g), pixel.r, pixel.b, 0.0);
    // I have no idea what pixel.x/y actually are (not coordinates)
    // newpixel = vec4(pixel.x, 0, pixel.y, pixel.x+pixel.y); // greyscale
    gl_FragColor = newpixel;
}



