#include "Scene_renderer.h"
// local
#include "Consts.h"
#include "Mesh_generator.h"
#include "Matrix_lib.h"
#include "Shader.h"
// boost
#include <boost/numeric/ublas/assignment.hpp>
// std
#include <stdexcept>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
// ImGui
#include "imgui.h"

//using namespace boost::numeric::odeint;
using namespace boost::numeric::ublas;

Scene_renderer::Scene_renderer()
    : tesseract_thickness_(1.f)
    , curve_thickness_(1.f)
    , sphere_diameter_(1.f)
    , number_of_animations_(6)
    , optimize_performance_(true)
    , visibility_mask_(0)
    , scene_pos(0, 0)
    , scene_size(0, 0)
{
}

Scene_renderer::Scene_renderer(std::shared_ptr<Scene_state> state)
    : Scene_renderer()
{
    set_state(state);
}

void Scene_renderer::load_shaders()
{
#ifdef __EMSCRIPTEN__
    mesh_shader_ids.program_id = Shader::load_shaders(
        "assets/Diffuse_ES.vert",
        "assets/Diffuse_ES.frag");
    screen_shader_ids.program_id = Shader::load_shaders(
        "assets/Diffuse_paint_ES.vert",
        "assets/Diffuse_paint_ES.frag");
#else
    mesh_shader_ids.program_id = Shader::load_shaders(
        "assets/Diffuse.vert",
        "assets/Diffuse.frag");
    screen_shader_ids.program_id = Shader::load_shaders(
        "assets/Diffuse_paint.vert",
        "assets/Diffuse_paint.frag");
#endif    

    glUseProgram(mesh_shader_ids.program_id);
    mesh_shader_ids.proj_mat_id =
        glGetUniformLocation(mesh_shader_ids.program_id, "projMatrix");
    mesh_shader_ids.mv_mat_id =
        glGetUniformLocation(mesh_shader_ids.program_id, "mvMatrix");
    mesh_shader_ids.normal_mat_id =
        glGetUniformLocation(mesh_shader_ids.program_id,"normalMatrix");
    mesh_shader_ids.light_pos_id =
        glGetUniformLocation(mesh_shader_ids.program_id, "lightPos");

    mesh_shader_ids.vertex_attrib_id =
        glGetAttribLocation(mesh_shader_ids.program_id, "vertex");
    mesh_shader_ids.normal_attrib_id =
        glGetAttribLocation(mesh_shader_ids.program_id, "normal");
    mesh_shader_ids.color_attrib_id =
        glGetAttribLocation(mesh_shader_ids.program_id, "color");
    
    glUseProgram(screen_shader_ids.program_id);
    screen_shader_ids.proj_mat_id =
        glGetUniformLocation(screen_shader_ids.program_id, "projMatrix");

    screen_shader_ids.vertex_attrib_id =
        glGetAttribLocation(screen_shader_ids.program_id, "vertex");
    screen_shader_ids.color_attrib_id =
        glGetAttribLocation(screen_shader_ids.program_id,  "color");
}

void Scene_renderer::set_state(std::shared_ptr<Scene_state> state)
{
    state_ = state;
}

void Scene_renderer::update_redering_region(glm::ivec2 pos,
                                            glm::ivec2 size)
{
    scene_pos = pos;
    scene_size = size;
}

void Scene_renderer::render()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /*glUseProgram(screen_shader_ids.program_id);

    // On-screen rendering
    glm::mat4 proj_ortho = glm::ortho(-1.f, 1.f, -1.f, 1.f);
    glUniformMatrix4fv(screen_shader_ids.proj_mat_id,
                       1,
                       GL_FALSE,
                       glm::value_ptr(proj_ortho));

    Line_2D line;
    line.start_pos = glm::vec2(-1.f, -1.f);
    line.end_pos   = glm::vec2(1.f, 1.f);
    line.width     = 1.;
    line.color     = glm::vec4(1.f, 0.f, 0.f, 0.2f);

    glLineWidth(5.f);
    draw_line_geometry(create_line_geometry(line));*/

    if(state_            == nullptr ||
       state_->tesseract == nullptr ||
       state_->curve     == nullptr)
    {
        return;
    }

    glUseProgram(mesh_shader_ids.program_id);

    glm::mat4 proj_mat = glm::perspective(
        state_->fov_y,
        (float)scene_size[0] / scene_size[1],
        0.1f,
        100.f);
    auto camera_mat = glm::translate(glm::mat4(1.f), state_->camera_3D);
    glm::vec3 light_pos(0.f, 0.f, 70.f);
    auto world_mat = glm::toMat4(state_->rotation_3D);
    auto norm_mat = glm::transpose(glm::inverse(glm::mat3(world_mat)));

    glUniformMatrix4fv(mesh_shader_ids.proj_mat_id,
                       1,
                       GL_FALSE,
                       glm::value_ptr(proj_mat));
    glUniformMatrix4fv(mesh_shader_ids.mv_mat_id,
                       1,
                       GL_FALSE,
                       glm::value_ptr(camera_mat * world_mat));
    glUniformMatrix3fv(mesh_shader_ids.normal_mat_id,
                       1,
                       GL_FALSE,
                       glm::value_ptr(norm_mat));
    glUniform3fv(mesh_shader_ids.light_pos_id,
                 1,
                 glm::value_ptr(light_pos));





    back_geometry_.clear(); front_geometry_.clear();

    std::vector<double> anims =
        split_animation(state_->unfolding_anim, number_of_animations_);
    double hide_4D = anims[0],
           project_curve_4D = anims[1],
           unfold_4D = anims[2],
           hide_3D = anims[3],
           project_curve_3D = anims[4],
           unfold_3D = anims[5];

    //gui_.Renderer->remove_all_meshes();
    //gui_.distanceWarning->hide();
    //gui_.Renderer->show_labels(false);
    //gui_.Renderer->remove_annotation_points();

    auto rot_m = get_rotation_matrix();

    // Project the tesseract from 4D to 3D
    Wireframe_object projected_t = *state_->tesseract.get();
    project_to_3D(projected_t.get_vertices(), rot_m);

    // Choosing the high-resolution or the low-resolution curve
    Curve projected_c;

    // Array of curves used to project into 3D spaces
    std::vector<Curve> curves_3D;

    if(!optimize_performance_)
    {
        projected_c = *state_->curve.get();
        for(int i = 0; i < 8; ++i)
            curves_3D.push_back(*state_->curve.get());
    }
    else
    {
        projected_c = *state_->simple_curve.get();
        for(int i = 0; i < 8; ++i)
            curves_3D.push_back(*state_->simple_curve.get());
    }

    // Project curves from 4D to 3D
    project_to_3D(projected_c.get_vertices(), rot_m);

    // Animation unfolding the tesseract to the Dali-cross
    if(state_->unfolding_anim == 0)
    {
        // Draw tesseract
        if(state_->show_tesseract)
            draw_tesseract(projected_t);

        // Draw 4D curve
        if(state_->show_curve)
        {
            draw_curve(projected_c, 1.);
            draw_annotations(projected_c);
        }
    }
    else
    {
        std::vector<Cube> plots_3D = state_->tesseract->split();

        move_curves_to_3D_plots(project_curve_4D, curves_3D);

        if(unfold_4D > 0)
            tesseract_unfolding(unfold_4D, plots_3D, curves_3D);

        // Project 3D plots from 4D to 3D
        auto rot = get_rotation_matrix(unfold_4D);
        for(auto& p : plots_3D)
            project_to_3D(p.get_vertices(), rot);

        auto visibility_coeff = [this](int i) {
            if(visibility_mask_ == 0 || visibility_mask_ & 1 << i)
                return 1.;
            else
                return 0.1;
        };

        if(0 < hide_4D && project_curve_3D == 0)
        {
            // Draw 3D plots
            if(state_->show_tesseract)
            {
                for(size_t i = 0; i < plots_3D.size(); ++i)
                {
                    auto& c = plots_3D[i];
                    if(state_->use_simple_dali_cross && i != 1 &&
                       i != 2 && i != 5 && i != 7)
                    {
                        // Sometimes it is dangerous to draw to transparent
                        // objects with direrent at different animation stages,
                        // because they both are drawn first. Therefore we make
                        // this ckecup here to avoid possible drawing issues
                        if(hide_4D != 1)
                        {
                            draw_3D_plot(
                                c, visibility_coeff(i) * (1 - hide_4D));
                        }
                    }
                    else
                    {
                        draw_3D_plot(c, visibility_coeff(i) * (1. - hide_3D));
                    }
                }
            }

            // Draw curves
            if(state_->show_curve)
            {
                // for(auto& c : curves_3D)
                for(size_t i = 0; i < curves_3D.size(); ++i)
                {
                    if(state_->use_simple_dali_cross && i != 1 &&
                       i != 2 && i != 5 && i != 7)
                    {
                        continue;
                    }

                    auto c = curves_3D[i];
                    project_to_3D(c.get_vertices(), rot);

                    draw_curve(c, visibility_coeff(i) * (1. - hide_3D));
                    if(visibility_coeff(i) == 1. && hide_3D < 0.5)
                        draw_annotations(c);
                }
            }
        }

        // Create meshes for the cubes representign the Tesseract / Dali-cross
        if(hide_3D > 0)
        {
            // Get the source plots
            std::vector<Square> plots_2D = Cube::split(plots_3D);

            std::vector<Curve> curves_2D;
            curves_2D.push_back(curves_3D[5]);
            curves_2D.push_back(curves_3D[1]);
            curves_2D.push_back(curves_3D[7]);
            curves_2D.push_back(curves_3D[1]);
            curves_2D.push_back(curves_3D[7]);
            curves_2D.push_back(curves_3D[7]);

            move_curves_to_2D_plots(project_curve_3D, curves_2D);
            for(auto& c : curves_2D)
            {
                project_to_3D(c.get_vertices(), rot);
            }

            plots_unfolding(unfold_3D, plots_2D, curves_2D);

            // Draw 2D plots
            if(state_->show_tesseract)
            {
                for(auto& p : plots_2D)
                    draw_2D_plot(p);
            }

            // TODO: uncomment the line below (rewrite it first ;) )!
            /*gui_.Renderer->set_label_positions(
                plots_2D[0].get_vertices()[3],
                plots_2D[3].get_vertices()[0],
                plots_2D[5].get_vertices()[1]);*/

            // Draw 2D curves
            if(state_->show_curve)
            {
                // TODO: uncomment lines below (rewrite it first ;) )!
                /*if(unfold_3D == 1.)
                    gui_.Renderer->show_labels(true);*/

                for(auto& c : curves_2D)
                {
                    draw_curve(c, 1.);
                    draw_annotations(c);
                }
            }
        }
    }

    for(auto& g : front_geometry_)
        draw_mesh_geometry(g);

    for(auto& g : back_geometry_)
        draw_mesh_geometry(g);
}

void Scene_renderer::project_to_3D(
    boost::numeric::ublas::vector<double>& point,
    const boost::numeric::ublas::matrix<double>& rot_mat)
{
    boost::numeric::ublas::vector<double> tmp_vert =
        prod(point, rot_mat);
    tmp_vert = tmp_vert - state_->camera_4D;
    tmp_vert = prod(tmp_vert, state_->projection_4D);

    //if(tmp_vert(3) < 0)
    //    gui_.distanceWarning->show();
    assert(tmp_vert(3) > 0);

    tmp_vert(0) /= tmp_vert(4);
    tmp_vert(1) /= tmp_vert(4);
    tmp_vert(2) /= tmp_vert(4);
    // Important!
    // Original coordinates in tmp_vert(3) and tmp_vert(4) are kept!
    // It is required for the 4D perspective.

    point = tmp_vert;
}

void Scene_renderer::project_to_3D(
    std::vector<boost::numeric::ublas::vector<double>>& verts,
    const boost::numeric::ublas::matrix<double>& rot_mat)
{
    auto project = [&](boost::numeric::ublas::vector<double>& v)
    {
        project_to_3D(v, rot_mat);
    };
    std::for_each(verts.begin(), verts.end(), project);
}

namespace
{
glm::vec4 ColorToGlm(const Color& c, const float alpha)
{
    return glm::vec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, alpha);
}
} // namespace

void Scene_renderer::draw_tesseract(Wireframe_object& t)
{
    Mesh t_mesh;
    for(auto const& e : t.edges())
    {
        auto& current = t.get_vertices()[e.vert1];
        auto& next = t.get_vertices()[e.vert2];
        const glm::vec4 col = ColorToGlm(e.color, 1.f);

        Mesh_generator::cylinder(
            5,
            tesseract_thickness_ / current(3),
            tesseract_thickness_ / next(3),
            glm::vec3(current(0), current(1), current(2)),
            glm::vec3(next(0), next(1), next(2)),
            col,
            t_mesh);

    }

    for(unsigned int i = 0; i < t.get_vertices().size(); ++i)
    {
        double size_coef = 1.0f;

        auto const& v = t.get_vertices()[i];
        glm::vec3 pos(v(0), v(1), v(2));

        if(i == 0)
            Mesh_generator::sphere(
                16,
                16,
                size_coef * sphere_diameter_ / v(3),
                pos,
                glm::vec4(1.f, 0.f, 0.f, 1.f),
                t_mesh);
        else
            Mesh_generator::sphere(
                16,
                16,
                size_coef * sphere_diameter_ / v(3),
                pos,
                glm::vec4(0.59f, 0.59f, 0.59f, 1.f),
                t_mesh);

    }

    back_geometry_.emplace_back(std::move(create_mesh_geometry(t_mesh)));
}

void Scene_renderer::draw_curve(Curve& c, float opacity)
{
    const double marker_size = 3.f; //gui_.markerSize->value();

    auto log_speed = [](float speed) {
        return std::log2(3 * speed + 1) / 2;
    };

    auto get_speed_color =
        [&opacity, &log_speed, this](float normalized_speed) {
            float speed = log_speed(normalized_speed);
            return glm::vec4(
                ((1 - speed) * state_->get_color(Curve_low_speed).r +
                 speed * state_->get_color(Curve_high_speed).r) /
                    255.f,
                ((1 - speed) * state_->get_color(Curve_low_speed).g +
                 speed * state_->get_color(Curve_high_speed).g) /
                    255.f,
                ((1 - speed) * state_->get_color(Curve_low_speed).b +
                 speed * state_->get_color(Curve_high_speed).b) /
                    255.f,
                opacity);
        };

    // Curve
    Mesh curve_mesh;
    for(size_t i = 0; i < c.edges().size(); ++i)
    {
        const auto& e = c.edges()[i];

        auto& current = c.get_vertices()[e.vert1];
        auto& next = c.get_vertices()[e.vert2];

        // We are interested only in some interval of the curve
        if(state_->curve_selection &&
           !state_->curve_selection->in_range(c.get_time_stamp()[e.vert1]))
        {
            continue;
        }

        auto& stats = c.get_stats();
        double speed_coeff = (stats.speed[i] - stats.min_speed) /
                             (stats.max_speed - stats.min_speed);

        Mesh_generator::cylinder(
            5,
            curve_thickness_ / current(3),
            curve_thickness_ / next(3),
            glm::vec3(current(0), current(1), current(2)),
            glm::vec3(next(0), next(1), next(2)),
            get_speed_color(speed_coeff),
            curve_mesh);
    }
    // TODO: fix the line below
    //gui_.Renderer->add_mesh(curve_mesh, opacity < 1.);
    back_geometry_.emplace_back(std::move(create_mesh_geometry(curve_mesh)));


    /*boost::numeric::ublas::vector<double> marker = c.get_point(player_pos_);

    Mesh marker_mesh;
    Mesh_generator::sphere(
        16,
        16,
        marker_size / marker(3),
        glm::vec3(marker(0), marker(1), marker(2)),
        glm::vec4(1, 0, 0, 1),
        marker_mesh);

    //gui_.Renderer->add_mesh(marker_mesh);
    back_geometry_.push_back(std::make_unique<Geometry_engine>(marker_mesh));*/
}

void Scene_renderer::set_line_thickness(float t_thickness, float c_thickness)
{
    tesseract_thickness_ = t_thickness;
    curve_thickness_ = c_thickness;
}

void Scene_renderer::set_sphere_diameter(float diameter)
{
    sphere_diameter_ = diameter;
}

std::unique_ptr<Scene_renderer::Mesh_geometry>
Scene_renderer::create_mesh_geometry(const Mesh& m)
{
    std::unique_ptr<Mesh_geometry> geom = std::make_unique<Mesh_geometry>();
    geom->data_array.clear();
    geom->indices.clear();
    GLuint ind = 0;

    for(size_t i = 0; i < m.objects.size(); ++i)
    {
        const auto& obj = m.objects[i];
        const auto& c = m.colors[i];

        for(auto const& f : obj.faces)
        {
            size_t num_verts = f.size();
            size_t num_triangles = num_verts - 2;

            for(size_t i = 0; i < num_triangles; ++i)
            {
                if(i == 0)
                {
                    // If the current triangle is first in the face we add three
                    // pairs of vertices and normals to the array

                    // Vertex 1
                    {
                        Mesh_array vnc;
                        vnc.vert = glm::vec4(m.vertices[f[0].vertex_id], 1);
                        vnc.norm = m.normals[f[0].normal_id];
                        vnc.color = c;
                        geom->data_array.push_back(vnc);
                    }
                    // Vertex 2
                    {
                        Mesh_array vnc;
                        vnc.vert = glm::vec4(m.vertices[f[i + 1].vertex_id], 1);
                        vnc.norm = m.normals[f[i + 1].normal_id];
                        vnc.color = c;
                        geom->data_array.push_back(vnc);
                    }
                    // Vertex 3
                    {
                        Mesh_array vnc;
                        vnc.vert = glm::vec4(m.vertices[f[i + 2].vertex_id], 1);
                        vnc.norm = m.normals[f[i + 2].normal_id];
                        vnc.color = c;
                        geom->data_array.push_back(vnc);
                    }
                }
                else
                {
                    // If the current triangle is not the first in the face it
                    // is enough to add only the one pair of vertices and
                    // normals to the array

                    // Vertex 3
                    {
                        Mesh_array vnc;
                        vnc.vert = glm::vec4(m.vertices[f[i + 2].vertex_id], 1);
                        vnc.norm = m.normals[f[i + 2].normal_id];
                        vnc.color = c;
                        geom->data_array.push_back(vnc);
                    }
                }

                geom->indices.push_back(ind);         // Vertex 1
                geom->indices.push_back(ind + i + 1); // Vertex 2
                geom->indices.push_back(ind + i + 2); // Vertex 3
            }

            ind += num_verts;
        }
    }

    geom->init_buffers();
    return geom;
}

void Scene_renderer::draw_mesh_geometry(
    const std::unique_ptr<Mesh_geometry>& geom)
{
    glBindBuffer(GL_ARRAY_BUFFER, geom->array_buff_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buff_id);

    glEnableVertexAttribArray(mesh_shader_ids.vertex_attrib_id);
    glEnableVertexAttribArray(mesh_shader_ids.normal_attrib_id);
    glEnableVertexAttribArray(mesh_shader_ids.color_attrib_id );

    GLsizei stride = sizeof(Mesh_array);
    void* ptr1 = reinterpret_cast<void*>(4 * sizeof(GLfloat));
    void* ptr2 = reinterpret_cast<void*>(7 * sizeof(GLfloat));
    glVertexAttribPointer(mesh_shader_ids.vertex_attrib_id,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          stride, 0);
    glVertexAttribPointer(mesh_shader_ids.normal_attrib_id,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          stride, ptr1);
    glVertexAttribPointer(mesh_shader_ids.color_attrib_id,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          ptr2);

    glDrawElements(GL_TRIANGLES, geom->indices.size(), GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(mesh_shader_ids.vertex_attrib_id);
    glDisableVertexAttribArray(mesh_shader_ids.normal_attrib_id);
    glDisableVertexAttribArray(mesh_shader_ids.color_attrib_id );
}

std::unique_ptr<Scene_renderer::Line_geometry>
Scene_renderer::create_line_geometry(const Line_2D& line)
{
    std::unique_ptr<Line_geometry> geom = std::make_unique<Line_geometry>();

    geom->data_array.clear();
    geom->indices.clear();

    Line_array d1, d2;

    d1.vert = line.start_pos;
    d1.color = line.color;

    d2.vert = line.end_pos;
    d2.color = line.color;

    geom->data_array.push_back(d1);
    geom->data_array.push_back(d2);

    geom->indices.push_back(0);
    geom->indices.push_back(1);

    // Allocating buffers
    glBindBuffer(GL_ARRAY_BUFFER, geom->array_buff_id);
	glBufferData(GL_ARRAY_BUFFER,
                 geom->data_array.size() * sizeof(Line_array),
                 &geom->data_array[0],
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buff_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 geom->indices.size() * sizeof(GLuint),
                 &geom->indices[0],
                 GL_STATIC_DRAW);

    geom->init_buffers();
    return geom;
}

void Scene_renderer::draw_line_geometry(
    const std::unique_ptr<Line_geometry>& geom)
{
    glBindBuffer(GL_ARRAY_BUFFER, geom->array_buff_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->index_buff_id);

    glEnableVertexAttribArray(screen_shader_ids.vertex_attrib_id);
    glEnableVertexAttribArray(screen_shader_ids.color_attrib_id );

    GLsizei stride = sizeof(Line_array);
    void* ptr = reinterpret_cast<void*>(2 * sizeof(GLfloat));
    glVertexAttribPointer(screen_shader_ids.vertex_attrib_id,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          0);
    glVertexAttribPointer(screen_shader_ids.color_attrib_id,
                          4,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          ptr);

    glDrawElements(GL_LINES, geom->indices.size(), GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(screen_shader_ids.vertex_attrib_id);
    glDisableVertexAttribArray(screen_shader_ids.color_attrib_id );
}

std::vector<double> Scene_renderer::split_animation(double animation,
                                                    int    sections)
{
    double section_length = 1. / sections;

    std::vector<double> splited_animations(sections);

    int current_section =
        animation == 1. ? sections - 1 : (int)std::floor(animation * sections);

    for(int i = 0; i < sections; ++i)
    {
        if(i < current_section)
            splited_animations[i] = 1.;
        else if(i == current_section)
            splited_animations[i] = animation * sections - current_section;
        else
            splited_animations[i] = 0.;
    }

    return splited_animations;
}

void Scene_renderer::draw_annotations(Curve& c)
{
    // TODO: port the method

    /*auto annot_arrows = c.get_arrows(*curve_selection_.get());
    auto annot_dots = c.get_markers(*curve_selection_.get());

    // Draw annotation arrows
    for(auto& a : annot_arrows)
        gui_.Renderer->add_annotation(a);

    // Draw annotation dots
    for(auto& a : annot_dots)
        draw_point(a, QColor(0, 0, 0), 3.);*/
}

void Scene_renderer::move_curves_to_3D_plots(double coeff,
                                             std::vector<Curve>& curves)
{
    // Curve 1
    for(auto& v : curves[0].get_vertices())
        v(3) = v(3) + coeff * (state_->tesseract_size[3] / 2 - v(3));
    // Curve 2
    for(auto& v : curves[1].get_vertices())
        v(3) = v(3) + coeff * (-state_->tesseract_size[3] / 2 - v(3));
    // Curve 3
    for(auto& v : curves[2].get_vertices())
        v(2) = v(2) + coeff * (state_->tesseract_size[2] / 2 - v(2));
    // Curve 4
    for(auto& v : curves[3].get_vertices())
        v(2) = v(2) + coeff * (-state_->tesseract_size[2] / 2 - v(2));
    // Curve 5
    for(auto& v : curves[4].get_vertices())
        v(1) = v(1) + coeff * (-state_->tesseract_size[1] / 2 - v(1));
    // Curve 6
    for(auto& v : curves[5].get_vertices())
        v(1) = v(1) + coeff * (state_->tesseract_size[1] / 2 - v(1));
    // Curve 7
    for(auto& v : curves[6].get_vertices())
        v(0) = v(0) + coeff * (-state_->tesseract_size[0] / 2 - v(0));
    // Curve 8
    for(auto& v : curves[7].get_vertices())
        v(0) = v(0) + coeff * (state_->tesseract_size[0] / 2 - v(0));
}

void Scene_renderer::move_curves_to_2D_plots(
    double coeff,
    std::vector<Curve>& curves)
{
    // Curve 1
    for(auto& v : curves[0].get_vertices())
        v(2) = v(2) + coeff * (-state_->tesseract_size[2] / 2 - v(2));
    // Curve 2
    for(auto& v : curves[1].get_vertices())
        v(2) = v(2) + coeff * (-state_->tesseract_size[2] / 2 - v(2));
    // Curve 3
    for(auto& v : curves[2].get_vertices())
        v(2) = v(2) + coeff * (-state_->tesseract_size[2] / 2 - v(2));
    // Curve 4
    for(auto& v : curves[3].get_vertices())
        v(1) = v(1) + coeff * (-state_->tesseract_size[1] / 2 - v(1));
    // Curve 5
    for(auto& v : curves[4].get_vertices())
        v(1) = v(1) + coeff * (-state_->tesseract_size[1] / 2 - v(1));
    // Curve 6
    for(auto& v : curves[5].get_vertices())
        v(0) = v(0) + coeff * (1.5 * state_->tesseract_size[0] - v(0));
}

void Scene_renderer::tesseract_unfolding(
    double coeff,
    std::vector<Cube>& plots_3D,
    std::vector<Curve>& curves_3D)
{
    auto transform_3D_plot =
        [](Wireframe_object& c, matrix<double>& rot, vector<double> disp) {
            for(auto& v : c.get_vertices())
            {
                for(int i = 0; i < 5; ++i)
                    v(i) += disp(i);

                v = prod(v, rot);

                for(int i = 0; i < 5; ++i)
                    v(i) -= disp(i);
            }
        };

    // Cube and curve 1 and 5
    {
        auto rot = Matrix_lib::getYWRotationMatrix(-coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp1(5);
        disp1 <<= 0,
                  state_->tesseract_size[1] / 2,
                  0,
                  state_->tesseract_size[3] / 2,
                  0;

        transform_3D_plot(plots_3D[0], rot, disp1);
        transform_3D_plot(plots_3D[4], rot, disp1);

        const auto vert = plots_3D[4].get_vertices()[0];
        boost::numeric::ublas::vector<double> disp2(5);
        disp2 <<= 0, -vert(1), 0, -vert(3), 0;

        transform_3D_plot(plots_3D[0], rot, disp2);

        transform_3D_plot(curves_3D[4], rot, disp1);

        transform_3D_plot(curves_3D[0], rot, disp1);
        transform_3D_plot(curves_3D[0], rot, disp2);
    }
    // Cube and curve 3
    {
        auto rot = Matrix_lib::getZWRotationMatrix(coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= 0,
                 0,
                 -state_->tesseract_size[2] / 2,
                 state_->tesseract_size[3] / 2,
                 0;

        transform_3D_plot(plots_3D[2], rot, disp);

        transform_3D_plot(curves_3D[2], rot, disp);
    }
    // Cube and curve 4
    {
        auto rot = Matrix_lib::getZWRotationMatrix(-coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= 0,
                 0,
                 state_->tesseract_size[2] / 2,
                 state_->tesseract_size[3] / 2,
                 0;

        transform_3D_plot(plots_3D[3], rot, disp);

        transform_3D_plot(curves_3D[3], rot, disp);
    }
    // Cube and curve 6
    {
        auto rot = Matrix_lib::getYWRotationMatrix(coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= 0,
                 -state_->tesseract_size[1] / 2,
                 0,
                 state_->tesseract_size[3] / 2,
                 0;

        transform_3D_plot(plots_3D[5], rot, disp);

        transform_3D_plot(curves_3D[5], rot, disp);
    }
    // Cube and curve 7
    {
        auto rot = Matrix_lib::getXWRotationMatrix(coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= state_->tesseract_size[0] / 2,
                 0,
                 0,
                 state_->tesseract_size[3] / 2,
                 0;

        transform_3D_plot(plots_3D[6], rot, disp);

        transform_3D_plot(curves_3D[6], rot, disp);
    }
    // Cube and curve 8
    {
        auto rot = Matrix_lib::getXWRotationMatrix(-coeff * PI / 2);
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= -state_->tesseract_size[0] / 2,
                  0,
                  0,
                  state_->tesseract_size[3] / 2,
                  0;

        transform_3D_plot(plots_3D[7], rot, disp);

        transform_3D_plot(curves_3D[7], rot, disp);
    }
}

boost::numeric::ublas::matrix<double> Scene_renderer::get_rotation_matrix()
{
    auto m = Matrix_lib::getXYRotationMatrix(state_->xy_rot);
    m = prod(m, Matrix_lib::getYZRotationMatrix(state_->yz_rot));
    m = prod(m, Matrix_lib::getZXRotationMatrix(state_->zx_rot));
    m = prod(m, Matrix_lib::getXWRotationMatrix(state_->xw_rot));
    m = prod(m, Matrix_lib::getYWRotationMatrix(state_->yw_rot));
    m = prod(m, Matrix_lib::getZWRotationMatrix(state_->zw_rot));

    return m;
}

boost::numeric::ublas::matrix<double>
Scene_renderer::get_rotation_matrix(double view_straightening)
{
    double angle_xw = (1 - view_straightening) * state_->xw_rot;
    double angle_yw = (1 - view_straightening) * state_->yw_rot;
    double angle_zw = (1 - view_straightening) * state_->zw_rot;

    auto m = Matrix_lib::getXYRotationMatrix(state_->xy_rot);
    m = prod(m, Matrix_lib::getYZRotationMatrix(state_->yz_rot));
    m = prod(m, Matrix_lib::getZXRotationMatrix(state_->zx_rot));
    m = prod(m, Matrix_lib::getXWRotationMatrix(angle_xw));
    m = prod(m, Matrix_lib::getYWRotationMatrix(angle_yw));
    m = prod(m, Matrix_lib::getZWRotationMatrix(angle_zw));

    return m;
}

void Scene_renderer::draw_3D_plot(Cube& cube, double opacity)
{
    for(size_t i = 0; i < cube.edges().size(); ++i)
    {
        auto const& e = cube.edges()[i];

        Mesh t_mesh;

        auto& current = cube.get_vertices()[e.vert1];
        auto& next = cube.get_vertices()[e.vert2];
        const glm::vec4 col = ColorToGlm(e.color, opacity);

        Mesh_generator::cylinder(
            5,
            tesseract_thickness_ / current(3),
            tesseract_thickness_ / next(3),
            glm::vec3(current(0), current(1), current(2)),
            glm::vec3(next(0), next(1), next(2)),
            col,
            t_mesh);

        opacity < 1.0 ? front_geometry_.emplace_back(std::move(create_mesh_geometry(t_mesh)))
                      : back_geometry_.emplace_back(std::move(create_mesh_geometry(t_mesh)));
    }
}

void Scene_renderer::draw_2D_plot(Wireframe_object& plot)
{
    for(auto const& e : plot.edges())
    {
        Mesh t_mesh;

        auto& current = plot.get_vertices()[e.vert1];
        auto& next = plot.get_vertices()[e.vert2];
        const glm::vec4 col = ColorToGlm(e.color, 1.f);

        Mesh_generator::cylinder(
            5,
            tesseract_thickness_ / current(3),
            tesseract_thickness_ / next(3),
            glm::vec3(current(0), current(1), current(2)),
            glm::vec3(next(0), next(1), next(2)),
            col,
            t_mesh);

        back_geometry_.emplace_back(std::move(create_mesh_geometry(t_mesh)));
    }
}

void Scene_renderer::plots_unfolding(
    double coeff,
    std::vector<Square>& plots_2D,
    std::vector<Curve>& curves_2D)
{
    auto transform_3D_plot =
        [](Wireframe_object& c, matrix<double>& rot, vector<double>& disp) {
            for(auto& v : c.get_vertices())
            {
                // We have to create a copy vector of the size of four in order
                // to multiype to the 4x4 rotation matrix
                boost::numeric::ublas::vector<double> copy_v(4);
                copy_v <<= v(0), v(1), v(2), v(3);

                for(int i = 0; i < 4; ++i)
                    copy_v(i) += disp(i);

                copy_v = prod(copy_v, rot);

                for(int i = 0; i < 4; ++i)
                    copy_v(i) -= disp(i);

                v <<= copy_v(0), copy_v(1), copy_v(2), copy_v(3), 0;
            }
        };

    {
        auto anchor = plots_2D[1].get_vertices()[0];
        auto rot_axis =
            plots_2D[1].get_vertices()[1] - plots_2D[1].get_vertices()[0];
        auto rot = Matrix_lib::getRotationMatrix(
            coeff * PI / 2, rot_axis(0), rot_axis(1), rot_axis(2));
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= -anchor(0), -anchor(1), -anchor(2), 0, 0;

        transform_3D_plot(plots_2D[3], rot, disp);
        transform_3D_plot(plots_2D[4], rot, disp);
        transform_3D_plot(plots_2D[5], rot, disp);

        transform_3D_plot(curves_2D[3], rot, disp);
        transform_3D_plot(curves_2D[4], rot, disp);
        transform_3D_plot(curves_2D[5], rot, disp);
    }
    {
        auto anchor = plots_2D[2].get_vertices()[1];
        auto rot_axis =
            plots_2D[4].get_vertices()[2] - plots_2D[4].get_vertices()[1];
        auto rot = Matrix_lib::getRotationMatrix(
            coeff * PI / 2, rot_axis(0), rot_axis(1), rot_axis(2));
        boost::numeric::ublas::vector<double> disp(5);
        disp <<= -anchor(0), -anchor(1), -anchor(2), 0, 0;

        transform_3D_plot(plots_2D[5], rot, disp);

        transform_3D_plot(curves_2D[5], rot, disp);
    }
}
