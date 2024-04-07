#include <string>
#include <vector>

struct ShaderDefinition
{
    std::string name;
    std::string vertex_source;
    std::string fragment_source;
    std::vector<std::string> texture_names;
    std::vector<std::string> uniform_names;
};

const auto yuv_fixed_shader = ShaderDefinition{
    .name = "yuv_fixed",
    .vertex_source = R"(
        #version 330 core
        layout (location = 0) in vec2 vert_pos;
        layout (location = 1) in vec2 vert_uv;
        out vec2 frag_uv;
        void main()
        {
            frag_uv = vert_uv;
            gl_Position = vec4(vert_pos.x, vert_pos.y, 0.0, 1.0);
        }
    )",
    .fragment_source = R"(
        #version 330 core
        in vec2 frag_uv;
        out vec4 frag_color;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        uniform sampler2D tex_v;
        void main()
        {
            highp float y = texture(tex_y, frag_uv).r;
            highp float u = texture(tex_u, frag_uv).r - 0.5;
            highp float v = texture(tex_v, frag_uv).r - 0.5;
            highp float r = y + 1.28033 * v;
            highp float g = y - 0.21482 * u - 0.38059 * v;
            highp float b = y + 2.12798 * u;

            frag_color = vec4(r, g, b, 1.0);
        }
    )",
    .texture_names = {"tex_y", "tex_u", "tex_v"},
    .uniform_names = {},
};

const auto yuv_param_shader = ShaderDefinition{
    .name = "yuv_param",
    .vertex_source = R"(
        #version 330 core
        layout (location = 0) in vec2 vert_pos;
        layout (location = 1) in vec2 vert_uv;
        out vec2 frag_uv;
        void main()
        {
            frag_uv = vert_uv;
            gl_Position = vec4(vert_pos.x, vert_pos.y, 0.0, 1.0);
        }
    )",
    .fragment_source = R"(
        #version 330 core
        in vec2 frag_uv;
        out vec4 frag_color;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        uniform sampler2D tex_v;
        uniform mat3 mat_yuv;
        void main()
        {
            highp float y = texture(tex_y, frag_uv).r;
            highp float u = texture(tex_u, frag_uv).r - 0.5;
            highp float v = texture(tex_v, frag_uv).r - 0.5;
            highp vec3 rgb = mat_yuv * vec3(y, u, v);

            frag_color = vec4(rgb, 1.0);
        }
    )",
    .texture_names = {"tex_y", "tex_u", "tex_v"},
    .uniform_names = {"mat_yuv"},
};

const auto hdr_test_shader = ShaderDefinition{
    .name = "hdr_test",
    .vertex_source = R"(
        #version 330 core
        layout (location = 0) in vec2 vert_pos;
        layout (location = 1) in vec2 vert_uv;
        out vec2 frag_uv;
        void main()
        {
            frag_uv = vert_uv;
            gl_Position = vec4(vert_pos.x, vert_pos.y, 0.0, 1.0);
        }
    )",
    .fragment_source = R"(
        #version 330 core
        in vec2 frag_uv;
        out vec4 frag_color;
        void main()
        {
            highp float z = frag_uv.x;

            frag_color = vec4(vec3(z*1.0), 1.0);
        }
    )",
    .texture_names = {},
    .uniform_names = {},
};
