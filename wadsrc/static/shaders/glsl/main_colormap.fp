uniform int texturemode;
uniform sampler2D tex;

uniform vec4 objectcolor;
uniform vec3 colormapstart;
uniform vec3 colormaprange;

vec4 Process(vec4 color);


vec4 desaturate(vec4 texel)
{
	return texel;
}

vec4 getTexel(vec2 st)
{
	vec4 texel = texture2D(tex, st);
	
	//
	// Apply texture modes
	//
	if (texturemode == 3)
	{
		texel *=objectcolor;
		texel = vec4(1.0-texel.r, 1.0-texel.g, 1.0-texel.b, texel.a);
		return texel;
	}
	else if (texturemode == 4)
	{
		texel = vec4(1.0, 1.0, 1.0, texel.r);
	}
	else if (texturemode == 2) 
	{
		texel.a = 1.0;
	}
	else if (texturemode == 1) 
	{
		texel.rgb = vec3(1.0,1.0,1.0);
	}

	return texel*objectcolor;
}



void main()
{
	vec4 frag = Process(vec4(1.0,1.0,1.0,1.0));
	
	float gray = (frag.r * 0.3 + frag.g * 0.56 + frag.b * 0.14);	
	vec3 cm = colormapstart + gray * colormaprange;
	gl_FragColor = vec4(clamp(cm, 0.0, 1.0), frag.a) * gl_Color;
}
