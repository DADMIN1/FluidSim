// This shader does nothing
uniform sampler2D texture; // must still define texture otherwise it errors
void main() 
{
    // must use texture otherwise it gets optimized out and causes errors again
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = pixel;
}
