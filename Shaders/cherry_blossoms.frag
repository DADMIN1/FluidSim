uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    gl_FragColor = pixel + vec4(cos(3.14*pixel.r), -0.2*pixel.g, cos(1.57*pixel.b), pixel.a);
}
