module camera;

import ray; 
import random;

public struct Camera {
    public __init(float3 center, /*float3 right,*/ float3 up, float3 lookAt, float defocusAngle, float focusDist,float aspectRatio, int height, float fov) {
        m_center = center;
        // m_right = right;
        m_up = up;
        m_lookAt = lookAt;
        m_aspectRatio = aspectRatio;
        m_height = height;
        m_fov = fov;
        m_defocusAngle = defocusAngle;
        m_focusDist = focusDist;
        float h = tan(radians(fov) / 2);
        m_vpHeight = 2 * h * m_focusDist;
        float vpWidth = m_vpHeight * aspectRatio;

        // Camera coordinate vector u (up), v (right), w (-front)
        m_camW = normalize(m_center - m_lookAt);
        m_camU = normalize(cross(m_up, m_camW));
        m_camV = cross(m_camW, m_camU);

        float3 viewportU = vpWidth * m_camU;
        float3 viewportV = m_vpHeight * -m_camV;
        m_pixelDeltaU = viewportU / (m_height * aspectRatio);
        m_pixelDeltaV = viewportV / m_height;

        float3 vpUpperLeft = m_center - m_focusDist * m_camW - viewportU * 0.5 - viewportV * 0.5;
        m_pixel00Loc = vpUpperLeft + 0.5 * m_pixelDeltaU + 0.5 * m_pixelDeltaV;

        // Camera defocus disk basis vectors
        float defocusRadius = m_focusDist * tan(radians(defocusAngle / 2));
        m_defocusDiskU = m_camU * defocusRadius;
        m_defocusDiskV = m_camV * defocusRadius;
    }

    public Ray getRay(int2 texelCoord, inout Random rand) {
        // Construct a camera ray originating from the defocus disk and directed at a randomly sampled point around the pixel (texelCoord)
        float3 offset = rand.sample_square();
        float3 currentPixel = m_pixel00Loc + m_pixelDeltaU * (float(texelCoord.x) + offset.x) + m_pixelDeltaV * (float(texelCoord.y) + offset.y);

        float3 rayOrigin = (m_defocusAngle <= 0) ? m_center : rand.defocus_disk_sample(m_center, m_defocusDiskU, m_defocusDiskV);
        
        return {rayOrigin, currentPixel - rayOrigin};
    }
    
    float3 m_center; // look from
    // float3 m_right;
    float3 m_up;
    float3 m_lookAt;
    float  m_defocusAngle;         // Variation angle of rays through each pixel
    float  m_focusDist;            // Distance from camera lookfrom to plane of perfect focus
    float3 m_defocusDiskU;         // Defocus disk horizontal radius
    float3 m_defocusDiskV;         // Defocus disk vertical radius
    float3 m_camU, m_camV, m_camW; // camera coordiante frame

    float3 m_pixelDeltaU;
    float3 m_pixelDeltaV;
    float3 m_pixel00Loc;

    float m_aspectRatio;
    int   m_height;
    float m_vpHeight;
    float m_fov;
}
