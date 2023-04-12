#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 FragPos;


uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;

uniform float shininess;
uniform vec3 Ia = vec3(0.15f,0.15f,0.15f);

//diffuse
uniform vec3 I_d; // directional
uniform vec3 I_p; // point
uniform vec3 I_s; //spot

//light position
uniform vec3 lightPos_d;
uniform vec3 lightPos_p;
uniform vec3 lightPos_s;
uniform int cur_light_id; //represent type of lights

uniform float spot_cutoff;
uniform mat4 view_matrix;
uniform vec3 cameraPos;
uniform int per_vertex;



vec3 directional_light()
{
	vec3 N = normalize(vertex_normal); //normalized normal vector
	vec3 L = normalize(lightPos_d); //normalized light source direction
	vec3 R = reflect(-L, N); //normalized light source reflection vector
	vec3 V = normalize(cameraPos-FragPos); //normalized viewpoint direction vector

	vec3 ambient = Ia * Ka;
	vec3 diffuse = I_d * Kd * max(dot(N,L),0);
	vec3 specular = Ks * pow(max(dot(R,V),0), shininess);

	vec3 color = ambient + diffuse + specular;
	return color;
}

vec3 point_light()
{
	vec3 N = normalize(vertex_normal); //normalized normal vector
	vec3 V = normalize(cameraPos-FragPos);//normalized viewpoint direction vector
	vec3 L = normalize(lightPos_p - FragPos);
	vec3 H = normalize(L+V); //halfway vector
	

	vec3 ambient = Ia * Ka;
	vec3 diffuse = I_p * Kd * max(dot(N,L),0);
	vec3 specular = Ks * pow(max(dot(N,H),0), shininess);

	//calculate attenuation with respect to the distance between the light source and the object
	float d = length(lightPos_p - FragPos);
	float f_att = min(1/(0.01 + 0.8 * d + 0.1 * d * d),1);

	vec3 color = ambient + f_att * diffuse + f_att * specular;
	return color;
}


vec3 spot_light()
{
	vec3 N = normalize(vertex_normal); //normalized normal vector
	vec3 V = normalize(cameraPos-FragPos);//normalized viewpoint direction vector
	vec3 L = normalize(lightPos_s - FragPos);
	vec3 H = normalize(L+V);  //halfway vector
	

	//calculate attenuation with respect to the distance between the light source and the object
	float d = length(lightPos_s - FragPos);
	float f_att = min(1/(0.05 + 0.3 * d + 0.6 * d * d),1);

	//spotlight effect
	vec4 spot_direction = vec4(0.0f,0.0f,-1.0f,1.0f);
	vec3 direction = normalize((transpose(inverse(view_matrix)) * spot_direction).xyz);
	float arc = dot(-L, direction);
	if(spot_cutoff<=degrees(acos(arc)))
	{
		vec3 ambient = Ia * Ka;
		vec3 color = ambient;
		return color;
	}
	else
	{
		float spot_effect =  pow(max(arc,0),50);
		vec3 ambient = Ia * Ka;
		vec3 diffuse = I_s * Kd * max(dot(N,L),0);
		vec3 specular = Ks * pow(max(dot(N,H),0), shininess);
		vec3 color = ambient + f_att * spot_effect * diffuse + f_att * spot_effect * specular;
		return color;
	}

}

void main() {
	// [TODO]
	//FragColor = vec4(vertex_normal, 1.0f);
	if(per_vertex == 1)
	{
		vec3 color = vec3(0,0,0);
		if(cur_light_id == 0)
		{
			//color += point_light();
			color += directional_light();
		}
		else if(cur_light_id == 1)
		{
			color += point_light();
		}
		else if(cur_light_id == 2)
		{
			color += spot_light();
		}
		FragColor = vec4 (color ,1.0);
	}
	else
	{
		FragColor = vec4 (vertex_color ,1.0);
	}
}
