uniform sampler2D daytex, nighttex;

varying vec3 normal, halfVect, lightDir;
varying vec4 ambient, diffuse;

void main()
{
    /* Perfragment normal and halfvector */
    vec3 normal_, halfVect_;
    float NdotL, NdotHV;
    
    vec4 daytexel = texture2D (daytex, gl_TexCoord[0].st);
    vec4 nighttexel = texture2D (nighttex, gl_TexCoord[0].st);
    
    normal_ = normalize (normal);
    
    NdotL = max (dot (normal_, lightDir), 0.0);
    
    vec4 color;
        
    if (NdotL == 0)
	color = 0.5 * nighttexel;
    else
    {
	float coeff = smoothstep (0.6,1,1-NdotL);
	color = NdotL * diffuse * daytexel + min(coeff, 0.5) * nighttexel;
	//color = NdotL * diffuse * daytexel + min((1-NdotL),0.6)*nighttexel;
    }
    //color = ambient + NdotL*diffuse*daytexel;
    
    if (NdotL > 0 && (daytexel.b>0.1 && daytexel.r<0.1) || (daytexel.r>0.9 && daytexel.g>0.9 && daytexel.b>0.9))
    {
	halfVect_ = normalize (halfVect);
	NdotHV = max (dot (normal_, halfVect_), 0.0);
	
	color += gl_LightSource[1].specular * gl_FrontMaterial.specular * pow(NdotHV, gl_FrontMaterial.shininess);
    }
    
    gl_FragColor = color;
}