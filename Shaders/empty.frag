// This shader does nothing
uniform sampler2D texture; // must still define texture otherwise it errors
void main() 
{
    // must use texture otherwise it gets optimized out and causes errors again
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = pixel;
    //gl_FragColor.a = 0.0;
    //discard;
}

/* the difference between return and discard is this:
discard throws away all of the current work (gl_FragColor) and returns;
whereas an early return will keep the current value of gl_FragColor */
