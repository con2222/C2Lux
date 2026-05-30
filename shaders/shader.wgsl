struct Uniforms {
    modelMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    projectionMatrix: mat4x4f,
    color: vec4f,
    cameraPosition: vec3f,
    time: f32,
};

struct LightData {
    direction: vec3<f32>,
    ambient: f32,
    intensity: f32,
    shininess: f32,
    specularStrength: f32,
};

const pi: f32 = 3.14159265359;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
    @location(3) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(3) worldPos: vec3<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vs_main(model: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    
    let mvp = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix;

    out.clip_position = mvp * vec4<f32>(model.position, 1.0);
    
    //out.color = uniforms.color.xyz;
    out.color = model.color;
    out.uv = model.uv;
    out.normal = model.normal;
    out.worldPos = (uniforms.modelMatrix * vec4<f32>(model.position, 1.0)).xyz;

    return out;
}

@group(1) @binding(0) var<uniform> lightData: LightData;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    let ambientOcclusion = 0.1f;
    let normal = normalize(in.normal);
    let lightDir = normalize(lightData.direction);

    let viewDir = normalize(uniforms.cameraPosition - in.worldPos);
    let reflectDir = reflect(-lightDir, normal);

    let specular = pow(max(dot(viewDir, reflectDir), 0.0), lightData.shininess);
    let diffuse = max(dot(normal, lightDir), 0.0);


    let finalColor =  (lightData.ambient + (diffuse + lightData.specularStrength * specular) * lightData.intensity) * in.color;
    return vec4<f32>(finalColor, 1.0);
}
