uniform sampler2D texture;

void main() 
{
    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);
    
    //gl_FragColor = vec4(cos(1.57*pixel.r), pixel.g, cos(3.14*pixel.b), pixel.a); // good alt
    gl_FragColor = vec4(cos(1.57*pixel.r), sin(pixel.g), cos(3.14*2*pixel.b), pixel.a);
}
