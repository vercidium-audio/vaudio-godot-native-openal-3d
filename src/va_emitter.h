#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>

extern "C"
{
#include <vaudio.h>
}

using namespace godot;

class ALFilter;
class ALReverbEffect;

namespace va_godot
{

class VAWorld;
class VAVisualisation;

class VAEmitter : public Node3D
{
    GDCLASS(VAEmitter, Node3D);

protected:
    VAWorld *va_world = nullptr;
    ::VAEmitter *emitter = nullptr;

    void create_emitter();
    void remove_emitter();
    void apply_properties_to_handle();

    // Removes the handle from the world and detaches it from this node, even if its removal is still pending
    void release_emitter();

    // Invoked once a VAWorld is found / when leaving the tree. Listeners override these to go through VAWorld's shared listener handle instead of owning one.
    virtual void attach_to_world();
    virtual void detach_from_world();

private:
    int reverb_ray_count = 0;
    int reverb_bounce_count = 0;
    float reverb_energy_cap = 0.15f;
    float minimum_reverb_energy = 0.01f;
    float max_volume = 1.0f;
    int max_echogram_time = 5000;
    int echogram_granularity = 100;
    bool affects_grouped_eax = false;
    bool keep_reverb_tail_alive = true;
    bool has_relative_reverb = false;
    float relative_reverb_inner_threshold = 0.6f;
    float relative_reverb_outer_threshold = 0.8f;
    bool use_listener_reverb = false;

    int occlusion_ray_count = 0;
    int occlusion_bounce_count = 0;
    float occlusion_energy_cap = 0.15f;
    float minimum_occlusion_energy = 0.01f;
    int permeation_ray_count = 0;
    int permeation_bounce_count = 0;
    float permeation_energy_cap = 0.15f;
    float minimum_permeation_energy = 0.01f;

    int ambient_occlusion_ray_count = 0;
    int ambient_occlusion_bounce_count = 0;
    float ambient_occlusion_energy_cap = 0.15f;
    float minimum_ambient_occlusion_energy = 0.01f;
    int ambient_permeation_ray_count = 0;
    int ambient_permeation_bounce_count = 0;
    float ambient_permeation_energy_cap = 0.15f;
    float minimum_ambient_permeation_energy = 0.01f;

    int type = 0;
    int trail_refresh_count = 16;
    float refresh_distance_threshold = 1.0f;
    int scattering_seed = 0;
    bool clamp_position = true;

    bool raytrace_once = false;
    bool raytrace_once_removed = false;

    bool random_trail_color = false;
    Color trail_color = Color(1.0f, 1.0f, 1.0f, 25.0f / 255.0f);
    Color reverb_color = Color(27.0f / 255.0f, 247.0f / 255.0f, 255.0f / 255.0f, 51.0f / 255.0f);
    Color occlusion_color = Color(113.0f / 255.0f, 255.0f / 255.0f, 164.0f / 255.0f, 51.0f / 255.0f);
    Color permeation_color = Color(255.0f / 255.0f, 127.0f / 255.0f, 42.0f / 255.0f, 51.0f / 255.0f);
    Color ambient_permeation_color = Color(255.0f / 255.0f, 204.0f / 255.0f, 0.0f / 255.0f, 51.0f / 255.0f);

    ALFilter *filter = nullptr;

    float ambient_filter_gain_lf = 1.0f;
    float ambient_filter_gain_hf = 1.0f;
    bool ambient_filter_ready = false;

    ALReverbEffect *effect = nullptr;

    VAVisualisation *visualisation = nullptr;

    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

    void apply_raytracing_results();

    static void on_raytracing_complete_trampoline(::VAEmitter *emitter);
    static void on_raytraced_by_another_emitter_trampoline(::VAEmitter *source, ::VAEmitter *target);
    static void on_removed_trampoline(::VAEmitter *emitter);

    void on_raytraced_by_another_emitter(::VAEmitter *other);

    void on_emitter_removed(::VAEmitter *handle);

protected:
    static void _bind_methods();
    void _validate_property(PropertyInfo &p_property) const;

public:
    // Destroys the orphaned handles owned by this world, called from ~VAWorld after vaWorldDestroy has unlinked them (e.g. ones still waiting on a reverb tail), so a late OnRemoved can't reach a freed VAWorld
    static void destroy_orphaned_handles(VAWorld *world);

    VAEmitter();
    ~VAEmitter();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    // Compatibility shim: forwards the pre-1.9.0 "refresh_ray_count" property to trail_refresh_count so existing .tscn/.tres files keep loading.
    bool _set(const StringName &p_name, const Variant &p_value);

    virtual bool is_main_listener() const
    {
        return false;
    }

    bool get_raytrace_once() const;
    void set_raytrace_once(bool value);

    ::VAEmitter *get_handle() const
    {
        return emitter;
    }

    bool is_raytraced() const;

    // True once the listener has raytraced this emitter and, if it casts reverb rays and affects grouped EAX, it has cast its own reverb rays too. Sources don't play until then
    bool is_ready_to_play() const;

    int get_grouped_eax_index() const;

    Vector3 get_va_position() const;

    bool get_within_world_bounds() const;

    Dictionary get_eax_debug_info() const;

    void add_target(VAEmitter *target);
    bool has_raytraced_target(VAEmitter *target) const;
    VALowPassFilter get_target_filter(VAEmitter *target) const;

    bool is_ambient_filter_ready() const
    {
        return ambient_filter_ready;
    }

    float get_ambient_filter_gain_lf() const
    {
        return ambient_filter_gain_lf;
    }

    float get_ambient_filter_gain_hf() const
    {
        return ambient_filter_gain_hf;
    }

    ALFilter *get_filter() const
    {
        return filter;
    }

    ALReverbEffect *get_effect() const
    {
        return effect;
    }

    VAVisualisation *get_visualisation() const
    {
        return visualisation;
    }

    void set_visualisation(VAVisualisation *value)
    {
        visualisation = value;
    }

    int get_reverb_ray_count() const;
    void set_reverb_ray_count(int value);
    int get_reverb_bounce_count() const;
    void set_reverb_bounce_count(int value);
    float get_reverb_energy_cap() const;
    void set_reverb_energy_cap(float value);
    float get_minimum_reverb_energy() const;
    void set_minimum_reverb_energy(float value);
    float get_max_volume() const;
    void set_max_volume(float value);
    int get_max_echogram_time() const;
    void set_max_echogram_time(int value);
    int get_echogram_granularity() const;
    void set_echogram_granularity(int value);
    bool get_affects_grouped_eax() const;
    void set_affects_grouped_eax(bool value);
    bool get_keep_reverb_tail_alive() const;
    void set_keep_reverb_tail_alive(bool value);
    bool get_has_relative_reverb() const;
    void set_has_relative_reverb(bool value);
    float get_relative_reverb_inner_threshold() const;
    void set_relative_reverb_inner_threshold(float value);
    float get_relative_reverb_outer_threshold() const;
    void set_relative_reverb_outer_threshold(float value);
    bool get_use_listener_reverb() const;
    void set_use_listener_reverb(bool value);

    int get_occlusion_ray_count() const;
    void set_occlusion_ray_count(int value);
    int get_occlusion_bounce_count() const;
    void set_occlusion_bounce_count(int value);
    float get_occlusion_energy_cap() const;
    void set_occlusion_energy_cap(float value);
    float get_minimum_occlusion_energy() const;
    void set_minimum_occlusion_energy(float value);
    int get_permeation_ray_count() const;
    void set_permeation_ray_count(int value);
    int get_permeation_bounce_count() const;
    void set_permeation_bounce_count(int value);
    float get_permeation_energy_cap() const;
    void set_permeation_energy_cap(float value);
    float get_minimum_permeation_energy() const;
    void set_minimum_permeation_energy(float value);

    int get_ambient_occlusion_ray_count() const;
    void set_ambient_occlusion_ray_count(int value);
    int get_ambient_occlusion_bounce_count() const;
    void set_ambient_occlusion_bounce_count(int value);
    float get_ambient_occlusion_energy_cap() const;
    void set_ambient_occlusion_energy_cap(float value);
    float get_minimum_ambient_occlusion_energy() const;
    void set_minimum_ambient_occlusion_energy(float value);
    int get_ambient_permeation_ray_count() const;
    void set_ambient_permeation_ray_count(int value);
    int get_ambient_permeation_bounce_count() const;
    void set_ambient_permeation_bounce_count(int value);
    float get_ambient_permeation_energy_cap() const;
    void set_ambient_permeation_energy_cap(float value);
    float get_minimum_ambient_permeation_energy() const;
    void set_minimum_ambient_permeation_energy(float value);

    int get_visualisation_ray_count() const;
    void set_visualisation_ray_count(int value);
    int get_visualisation_bounce_count() const;
    void set_visualisation_bounce_count(int value);
    int get_visualisation_update_frequency() const;
    void set_visualisation_update_frequency(int value);

    int get_type() const;
    void set_type(int value);
    int get_trail_refresh_count() const;
    void set_trail_refresh_count(int value);
    float get_refresh_distance_threshold() const;
    void set_refresh_distance_threshold(float value);
    int get_scattering_seed() const;
    void set_scattering_seed(int value);
    bool get_clamp_position() const;
    void set_clamp_position(bool value);

    bool get_random_trail_color() const;
    void set_random_trail_color(bool value);
    Color get_trail_color() const;
    void set_trail_color(const Color &value);
    Color get_reverb_color() const;
    void set_reverb_color(const Color &value);
    Color get_occlusion_color() const;
    void set_occlusion_color(const Color &value);
    Color get_permeation_color() const;
    void set_permeation_color(const Color &value);
    Color get_ambient_permeation_color() const;
    void set_ambient_permeation_color(const Color &value);
};

} // namespace va_godot
