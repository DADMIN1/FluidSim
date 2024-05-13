uniform sampler2D texture;

void main() 
{
    // lookup the pixel in the texture
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    
    
    // multiply it by the color
    gl_FragColor = gl_FragColor - cos(3.14 * pixel);
}