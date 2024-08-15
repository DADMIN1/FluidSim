// yellow -> red scheme; kind of inverse of 'red' shader
uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = cos(3.14 * pixel);
    gl_FragColor.a = pixel.a;
}
