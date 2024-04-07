#include <chrono>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <map>
#include <array>

#include "opengl.h"
#include "ffmpeg.h"
#include "shader.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

namespace chrono = std::chrono;

std::map<std::string, std::array<float, 9>> yuv_mats = {
    { "SDTV", {
        1.0f, 0.0f, 1.13983f,
        1.0f, -0.39465f, -0.58060f,
        1.0f, 2.03211f, 0.0f,
    }},
    { "HDTV", {
        1.0f, 0.0f, 1.28033f,
        1.0f, -0.21482f, -0.38059f,
        1.0f, 2.12798f, 0.0f,
    }},
    { "BT.709", {
        1.0f, 0.0f, 1.5748f,
        1.0f, -0.1873f, -0.4681f,
        1.0f, 1.8556f, 0.0f,
    }},
    { "JPEG", {
        1.0f, 0.0f, 1.402f,
        1.0f, -0.344136f, -0.714136f,
        1.0f, 1.772f, 0.0f,
    }},
};

struct ShaderProgram
{
    GLuint id;
    std::vector<GLuint> textures;
    std::vector<int> texture_locs; // Uniform location for ith texture
    std::vector<int> uniform_locs;

    ShaderProgram(const ShaderDefinition &def)
    {
        id = opengl::create_shader(def.vertex_source, def.fragment_source);
        glUseProgram(id);

        printf("New shader (%s) id=%d\n", def.name.c_str(), id);

        if (!def.texture_names.empty())
        {
            textures.resize(def.texture_names.size());
            glGenTextures(textures.size(), textures.data());

            for (const auto name : def.texture_names)
            {
                int loc = glGetUniformLocation(id, name.c_str());
                texture_locs.push_back(loc);
            }

            for (const auto name : def.uniform_names)
            {
                int loc = glGetUniformLocation(id, name.c_str());
                uniform_locs.push_back(loc);
            }

            for (size_t i = 0; i < textures.size(); i++)
            {
                glUniform1i(texture_locs[i], i);
            }
        }
    }

    ~ShaderProgram()
    {
        glDeleteProgram(id);
    }

    void update_texture(int index, int width, int height, int storage_format, int input_format, int type, const void *ptr)
    {
        glActiveTexture(GL_TEXTURE0 + index);
        glBindTexture(GL_TEXTURE_2D, textures[index]);
        glTexImage2D(GL_TEXTURE_2D, 0, storage_format, width, height, 0, input_format, type, ptr);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
};

class Player
{
private:
    // OpenGL state
    GLFWwindow *_window;
    GLuint _rect_vao, _rect_vbo;

    // Input data
    std::string _path;

    // Playback state
    size_t _frame_idx;
    bool _paused;

public:
    struct Params
    {
        int window_width, window_height;
        int opengl_version[2];
    };

    void run()
    {
        ffmpeg::AsyncReader reader(_path, 1);
        AVFrame *frame = nullptr;

        // Preload the first frame
        while (!frame)
        {
            frame = reader.next_frame();

            if (reader.eof())
                break;
        }

        int h_shift, v_shift;
        av_pix_fmt_get_chroma_sub_sample((AVPixelFormat)frame->format, &h_shift, &v_shift);

        chrono::high_resolution_clock clock;
        auto t0 = clock.now();

        int fps = 60;
        long frame_time_us = 1000000 / fps;

        ShaderProgram shader(yuv_param_shader);

        size_t yuv_mat_index = 0;
        glUniformMatrix3fv(shader.uniform_locs[0], 1, GL_TRUE, std::next(yuv_mats.begin(), yuv_mat_index)->second.data());

        struct KeyBinding
        {
            GLFWwindow *window;
            int key;
            bool pressed;

            KeyBinding(GLFWwindow *window_, int key_): window(window_), key(key_), pressed(false) {}

            bool clicked()
            {
                switch (glfwGetKey(window, key)) {
                case GLFW_PRESS:
                    if (!pressed)
                    {
                        pressed = true;
                        return true;
                    }
                    break;
                case GLFW_RELEASE:
                    pressed = false;
                    break;
                }

                return false;
            }
        };

        KeyBinding enter_key(_window, GLFW_KEY_ENTER);
        KeyBinding space_key(_window, GLFW_KEY_SPACE);

        // Main loop
        while (!glfwWindowShouldClose(_window))
        {
            // Process input
            if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(_window, true);

            if (enter_key.clicked())
            {
                yuv_mat_index = (yuv_mat_index + 1) % yuv_mats.size();
                auto kv = std::next(yuv_mats.begin(), yuv_mat_index);

                std::cout << "yuv_mat: " << kv->first << "\n";
                glUniformMatrix3fv(shader.uniform_locs[0], 1, GL_TRUE, kv->second.data());
            }

            if (space_key.clicked())
            {
                _paused = !_paused;
            }

            auto tnow = clock.now();

            if (chrono::duration_cast<chrono::microseconds>(tnow - t0).count() > frame_time_us && !_paused)
            {
                if (AVFrame *new_frame = reader.next_frame())
                {
                    av_frame_free(&frame);
                    frame = new_frame;
                    ++_frame_idx;

                    if (reader.eof())
                        break;
                }

                t0 = tnow;
            }

            // Rendering commands
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            int w = frame->width, h = frame->height;
            shader.update_texture(0, w, h, GL_RED, GL_RED, GL_UNSIGNED_BYTE, frame->data[0]);
            shader.update_texture(1, w >> h_shift, h >> v_shift, GL_RED, GL_RED, GL_UNSIGNED_BYTE, frame->data[1]);
            shader.update_texture(2, w >> h_shift, h >> v_shift, GL_RED, GL_RED, GL_UNSIGNED_BYTE, frame->data[2]);

            glUseProgram(shader.id);
            glBindVertexArray(_rect_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glfwSwapBuffers(_window);

            // Poll for and process events
            glfwPollEvents();
        }
    }

    Player(const std::string &path, const Params &params):
        _path(path),
        _paused(true)
    {
        _window = opengl::create_current_window(params.window_width, params.window_height, params.opengl_version);
        opengl::create_rect(&_rect_vao, &_rect_vbo);
    }

    ~Player()
    {
        glDeleteVertexArrays(1, &_rect_vao);
        glDeleteBuffers(1, &_rect_vbo);

        glfwTerminate();
    }
};

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_video_file>" << std::endl;
        return 1;
    }

    Player player(argv[argc - 1], {1280, 720, {3, 3}});
    player.run();

    return 0;
}

