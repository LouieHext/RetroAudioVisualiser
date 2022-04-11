#version 150

//created by Louie Hext - 03/2021
//Live Music Retro Visualiser

//domain warping from inigo Quilez - https://www.iquilezles.org/www/articles/warp/warp.htm
//dithering from - http://alex-charlton.com/posts/Dithering_on_the_GPU/
//noise from - https://weber.itn.liu.se/~stegu/jgt2011/supplement.pdf




//shader out
out vec4 outputColor; 

//shader in
uniform vec3 palette [8];

uniform vec2 u_resolution;
uniform float u_time;
uniform float u_scale;
uniform float u_timeScale;

uniform int numDistortion;
uniform int numFBM;
uniform float powerFBM;

//globals
 int paletteSize=8;
 //dithering matrix
 const int indexMatrix8x8[64] = int[](0,  32, 8,  40, 2,  34, 10, 42,
                                     48, 16, 56, 24, 50, 18, 58, 26,
                                     12, 44, 4,  36, 14, 46, 6,  38,
                                     60, 28, 52, 20, 62, 30, 54, 22,
                                     3,  35, 11, 43, 1,  33, 9,  41,
                                     51, 19, 59, 27, 49, 17, 57, 25,
                                     15, 47, 7,  39, 13, 45, 5,  37,
                                     63, 31, 55, 23, 61, 29, 53, 21);





//simple noise source 
//https://weber.itn.liu.se/~stegu/jgt2011/supplement.pdf
//---------------------------------------------------------
vec4 permute(vec4 x){
    return mod( x*((x * 34.0)+1.0 ), 289.0);
    }

vec4 taylorInvSqrt(vec4 r){
    return 1.79284291400159-0.85373472095314*r;
}

float snoise(vec3 v){
    const vec2  C=vec2  (1.0/6.0,1.0/3.0);
    const vec4 D=vec4 (0.0,0.5,1.0,2.0);
    //First corner
    vec3 i=floor(v+dot(v,C.yyy));
    vec3 x0=v-i+dot(i,C.xxx);
    //Other corners
    vec3 g=step(x0.yzx,x0.xyz);
    vec3 l=1.0-g;
    vec3 i1=min(g.xyz,l.zxy);
    vec3 i2=max(g.xyz,l.zxy);
    //x0=x0-0.+0.0*C
    vec3 x1=x0-i1+1.0*C.xxx;
    vec3 x2=x0-i2+2.0*C.xxx;
    vec3 x3=x0-1.+3.0*C.xxx;
    //Permutations
    i=mod(i,289.0);
    vec4 p=permute(permute(permute(
    i.z+vec4 (0.0,i1.z,i2.z,1.0))
    +i.y+vec4 (0.0,i1.y,i2.y,1.0))
    +i.x+vec4 (0.0,i1.x,i2.x,1.0));
    //Gradients from 7x 7points over a square,mapped on to an octahedron
    float n_=1.0/7.0;
    vec3 ns=n_*D.wyz-D.xzx;
    vec4 j=p-49.0*floor(p*ns.z*ns.z);//mod(p,7*7)
    vec4 x_=floor(j*ns.z);
    vec4 y_=floor(j-7.0*x_);//mod(j,7)
    vec4 x=x_*ns.x+ns.yyyy;
    vec4 y=y_*ns.x+ns.yyyy;
    vec4 h=1.0-abs(x)-abs(y);
    vec4 b0=vec4 (x.xy,y.xy);
    vec4 b1=vec4 (x.zw,y.zw);
    //vec4 s0=vec4 (lessThan(b0,0.0))*2.0-1.0;
    //vec4 s1=vec4 (lessThan(b1,0.0))*2.0-1.0;
    vec4 s0=floor(b0)*2.0+1.0;
    vec4 s1=floor(b1)*2.0+1.0;
    vec4 sh=-step(h,vec4 (0.0));
    vec4 a0=b0.xzyw+s0.xzyw*sh.xxyy;
    vec4 a1=b1.xzyw+s1.xzyw*sh.zzww;

    vec3 p0=vec3 (a0.xy,h.x);
    vec3 p1=vec3 (a0.zw,h.y);
    vec3 p2=vec3 (a1.xy,h.z);
    vec3 p3=vec3 (a1.zw,h.w);
    //Normalise gradients
    vec4 norm=taylorInvSqrt(vec4 (dot(p0,p0),dot(p1,p1),dot(p2,p2),
    dot(p3,p3)));
    p0*=norm.x;
    p1*=norm.y;
    p2*=norm.z;
    p3*=norm.w;
    //Mix final noisevalue
    vec4 m=max(0.6-vec4 (dot(x0,x0),dot(x1,x1),dot(x2,x2),dot(x3,x3)
    ),0.0);
    m=m*m;
    return 42.0*dot(m*m,vec4 (dot(p0,x0),dot(p1,x1),
    dot(p2,x2),dot(p3,x3)));
}

//---------------------------------------------------------


//custom FBM funciton that uses inputs from OF
//combining higher frequency lower amplitude noise fields
float fbm(vec3 v){
	float value = 0;
	vec2 v_pos=vec2(v.x,v.y); //getting positional info
	
	for(int i = 0; i <numFBM ; i++){
         value+=(snoise(vec3( v_pos*pow(2.0,float(i)) , v.z ) ))*pow(2.0,(0.01+12.*powerFBM)*float(-i));
    }   
	return value;
}




//dithering and colour functions from
//http://alex-charlton.com/posts/Dithering_on_the_GPU/
//and
//https://www.chilliant.com/rgb2hsv.html
//---------------------------------------------------------

//globals								 
  float Epsilon = 1e-10;

  //helpers
  float indexValue() {
    int x = int(mod(gl_FragCoord.x, 8));
    int y = int(mod(gl_FragCoord.y, 8));
    return indexMatrix8x8[(x + y * 8)] / 64.0;
}

 
  vec3 RGBtoHCV( vec3 RGB)
  {
    // Based on work by Sam Hocevar and Emil Persson
    vec4 P = (RGB.g < RGB.b) ? vec4(RGB.bg, -1.0, 2.0/3.0) : vec4(RGB.gb, 0.0, -1.0/3.0);
    vec4 Q = (RGB.r < P.x) ? vec4(P.xyw, RGB.r) : vec4(RGB.r, P.yzx);
    float C = Q.x - min(Q.w, Q.y);
    float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
    return vec3(H, C, Q.x);
  }

vec3 HUEtoRGB(float H)
  {
    float R = abs(H * 6 - 3) - 1;
    float G = 2 - abs(H * 6 - 2);
    float B = 2 - abs(H * 6 - 4);
    return clamp(vec3(R,G,B), 0, 1);
  }

vec3 HSLtoRGB(vec3 HSL)
  {	
    vec3 RGB = HUEtoRGB(HSL.x);
    float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
    return (RGB - 0.5) * C + HSL.z;
  }

 vec3 RGBtoHSL(vec3 RGB)
  {
    vec3 HCV = RGBtoHCV(RGB);
    float L = HCV.z - HCV.y * 0.5;
    float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
    return vec3(HCV.x, S, L);
  }


float hueDistance(float h1, float h2) {
    float diff = abs((h1 - h2));
    return min(abs((1.0 - diff)), diff);
}

//methods
vec3[2] closestColors(float hue) {
	//gets the closest colour (based on hue distance) from the pallete to the pixels current colour
    vec3 ret[2];
    vec3 closest = vec3(-2, -2, -2); //def values
    vec3 secondClosest = vec3(-2, 0, 0);
    vec3 temp;
    for (int i = 0; i < paletteSize; ++i) {
        temp = RGBtoHSL(palette[i]);
        float tempDistance = hueDistance(temp.x, hue);
        if (tempDistance < hueDistance(closest.x, hue)) {
            secondClosest = closest;
            closest = temp;
        } else {
            if (tempDistance < hueDistance(secondClosest.x, hue)) {
                secondClosest = temp;
            }
        }
    }
    ret[0] = closest;
    ret[1] = secondClosest;
    return ret;
}




const float lightnessSteps = 4.0;

float lightnessStep(float l) {
    /* Quantize the lightness to one of `lightnessSteps` values */
    return floor((0.5 + l * lightnessSteps)) / lightnessSteps;
}

//main dithering procesdure
vec3 dither(vec3 color) {
    vec3 hsl = RGBtoHSL(color);
    vec3 cs[2] = closestColors(hsl.x); //getting closest colours
    vec3 c1 = cs[0];
    vec3 c2 = cs[1];
    float d = indexValue();
    float hueDiff = hueDistance(hsl.x, c1.x) / hueDistance(c2.x, c1.x); //hue distance

    float l1 = lightnessStep(max((hsl.z - 0.125), 0.0));
    float l2 = lightnessStep(min((hsl.z + 0.124), 1.0));
    float lightnessDiff = (hsl.z - l1) / (l2 - l1); //lightness difference based on hue dist
	
    vec3 resultColor = (hueDiff < d) ? c1 : c2;
    resultColor.z = (lightnessDiff < d) ? l1 : l2;
    return HSLtoRGB(resultColor);
}


void main(){        

	//redefine uniforms for ease of use
    float scale = u_timeScale;
    float noiseScale=u_scale;

	//applying movement and scaling
    vec2 position =((noiseScale)) *(gl_FragCoord.xy/u_resolution-0.5);

    float value =(snoise(vec3(position,scale*u_time))); //base noise
    value=abs(1.0-value*value); //nicer visual

    float base =  0.1*(snoise(vec3(position,0.5*u_time)));  
	value+=fbm(vec3(position,scale*u_time)); //appying fbm to base noise

	//domain warping (f(p) -> f(f(p + dp)) effectively, but applied a few times)
    value=abs(1.0-pow(value,1.5)); 
    vec2 q= vec2(value,snoise(vec3(value+3.5,value +4.3,scale*u_time)));
    value+=snoise(vec3(value+3.0*q.x,value+4.3*q.y,scale*u_time));
    vec2 r= vec2(value,snoise(vec3(value+3.5,value +2.3,scale*u_time)));
    value+=snoise(vec3(value+2.0*r.x,value+5.3*r.y,scale*u_time));
    vec2 s= vec2(value,snoise(vec3(value+4.58,value +4.3,scale*u_time)));
    value+=snoise(vec3(value+2.7*s.x,value+3.9*s.y,scale*u_time));
   

   
   //colouring
   //random colours to mix from
    const vec4 white=vec4(0.7725, 0.2353,   0.2353, 1.0);
    const vec4 a=vec4(0.8118, 0.6627, 0.251, 1.0);
    const vec4 b=vec4(0.1098, 0.7686, 0.9333, 0.4);
    const vec4 c=vec4(0.3176, 0.6118, 0.8863, 0.9);
    const vec4 d=vec4(0.051, 0.2745, 0.6078, 1.0);

	//mixing colours based on domain warped noise values
	//some arbitary mixing functions
    vec4 colour=mix(white,a,1.0-value*value);
    colour=mix(colour,c,r.x-s.y);
    colour=mix(colour,d,s.x*q.x);
    colour=mix(colour,b,q.y*r.y);
    colour=colour*colour;
	
	//shader out
    outputColor =vec4( dither(vec3(colour.x,colour.y,colour.z) ) ,1.0);
}
