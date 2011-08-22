uniform sampler2D daytex, nighttex;

varying vec3 normal, halfVect, lightDir;
varying vec4 ambient, diffuse;

void main()
{
    /* Perfragment normal and halfvector */
    vec3 normal_, halfVect_;
    float NdotL, NdotHV;
    
    /* Texture data */
    vec4 daytexel = texture2D (daytex, gl_TexCoord[0].st);
    vec4 nighttexel = texture2D (nighttex, gl_TexCoord[0].st);
    
    normal_ = normalize (normal);
    
    NdotL = max (dot (normal_, lightDir), 0.0);
    
    vec4 color;
    
    /* Ambient and diffuse light with day texture */    
    color = (ambient + NdotL*diffuse) * daytexel;
    
    /* Display the night lights on top of the night side, with a little gradient.*/
    float coeff = smoothstep (0.9, 1, 1 - NdotL);
    color += nighttexel * coeff;
    
    
    if (NdotL > 0)
    {
	halfVect_ = normalize (halfVect);
	NdotHV = max (dot (normal_, halfVect_), 0.0);    
	/* Display a specular reflexion if we are on ocean or ice */
	if ((daytexel.b>0.1 && daytexel.r<0.2) || (daytexel.r>0.8 && daytexel.g>0.9 && daytexel.b>0.9))
	    color += gl_LightSource[1].specular * gl_FrontMaterial.specular * pow(NdotHV, gl_FrontMaterial.shininess);
	else
	    color += 0.3 * gl_LightSource[1].specular * gl_FrontMaterial.specular * pow(NdotHV, gl_FrontMaterial.shininess/4);
    }
    
    gl_FragColor = color;
}