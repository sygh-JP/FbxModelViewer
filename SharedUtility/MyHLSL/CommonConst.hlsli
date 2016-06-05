// Only UTF-8 or ASCII is available.

static const uint DOWN_SAMPLED_TEX_SIZE = 128;
static const uint COMPUTING_TEMP_WORK_SIZE = 256;
static const uint SHADOW_MAP_TEXTURE_SIZE = 1024;
//static const uint SHADOW_MAP_TEXTURE_SIZE = 2048;

// Do not define functions just for grayscale conversion because it is simpler to calculate dot product with rgb swizzle components.

static const float3 RgbToYFactor3F = { 0.29891f, 0.58661f, 0.11448f };
static const float4 RgbToYFactor4F = { 0.29891f, 0.58661f, 0.11448f, 0 };

static const float3x3 IdentityMatrix3x3F = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
static const float4x4 IdentityMatrix4x4F = { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } };

static const float F_PI = 3.14159265f;
