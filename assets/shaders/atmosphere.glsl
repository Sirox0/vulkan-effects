// modified version of https://github.com/patriciogonzalezvivo/lygia/blob/main/lighting/atmosphere.glsl

float rayleigh(const in float mu) {
    return 3. * (1. + mu*mu) / (16. * acos(-1.0));
}

float henyeyGreenstein(const in float mu) {
    return max(0.0, (1.0 - 0.76*0.76) / ((4. + acos(-1.0)) * pow(1.0 + 0.76*0.76 - 2.0 * 0.76 * mu, 1.5)));
}

vec3 atmosphere_pos(vec3 rayOrigin, vec3 rayDir, float dist, float ds) {
    return rayOrigin + rayDir * (dist + ds * 0.5);
}

float atmosphere_height(vec3 rayOrigin, vec3 rayDir, float dist,  float ds, inout vec2 density) {
    vec3 p = atmosphere_pos(rayOrigin, rayDir, dist, ds);
    float h = length(p) - 6371e3;

    density += exp(-h * vec2(125e-6, 833e-6)) * ds; // Rayleigh
    return h;
}

bool atmosphere_light(vec3 rayOrigin, vec3 rayDir, inout vec2 depth, uint lightSteps) {
    float t0 = 0.0;     // Atmosphere entry point 
    float t1 = 99999.0; // Atmosphere exit point

    float dist = 0.;
    float dstep = t1 / float(lightSteps);
    for (int i = 0; i < lightSteps; i++) {
        if (atmosphere_height(rayOrigin, rayDir, dist,  dstep, depth) <= 0.0)
            return false;

        dist += dstep;
    }

    return true;
}

vec3 atmosphere(vec3 rayOrigin, vec3 rayDir, vec3 sunDir, float sunPower, uint steps, uint lightSteps) {
    float t0 = 0.0;
    float t1 = 99999.0;

    float dstep = t1 / float(steps);
    vec2 depth = vec2(0.0);

    vec3 sumR = vec3(0.0, 0.0, 0.0);
    vec3 sumM = vec3(0.0, 0.0, 0.0);
    float dist = 0.0;
    for (int i = 0; i < steps; i++) {
        vec2 density = vec2(0.);

        atmosphere_height(rayOrigin, rayDir, dist, dstep, density);

        depth += density;

        vec2 light = vec2(0.);
        if (atmosphere_light(atmosphere_pos(rayOrigin, rayDir, dist, dstep), sunDir, light, lightSteps)) {
            vec3 attn = exp(-vec3(55e-7, 13e-6, 22e-6) * (depth.x + light.x)
                            -vec3(21e-6) * (depth.y + light.y));
            sumR += density.x * attn;
            sumM += density.y * attn;
        }

        dist += dstep;
    }

    float mu = dot(rayDir, sunDir);
    sumR *= rayleigh(mu) * vec3(55e-7, 13e-6, 22e-6);
    sumM *= henyeyGreenstein(mu) * vec3(21e-6);
    vec3 color = sunPower * (sumR + sumM);

    return color;
}

vec3 atmosphere(vec3 viewDir, vec3 sunDir, float sunPower, uint steps, uint lightSteps) {
    return atmosphere(vec3(0., 6371e3 + 1.0, 0.), viewDir, sunDir, sunPower, steps, lightSteps);
}