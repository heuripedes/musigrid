#include "libretro.h"

RETRO_API void retro_set_environment(retro_environment_t);
RETRO_API void retro_set_video_refresh(retro_video_refresh_t);
RETRO_API void retro_set_audio_sample(retro_audio_sample_t);
RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
RETRO_API void retro_set_input_poll(retro_input_poll_t);
RETRO_API void retro_set_input_state(retro_input_state_t);

/* Library global initialization/deinitialization. */
RETRO_API void retro_init(void);
RETRO_API void retro_deinit(void);

/* Must return RETRO_API_VERSION. Used to validate ABI compatibility
 * when the API is revised. */
RETRO_API unsigned retro_api_version(void);

/* Gets statically known system info. Pointers provided in *info
 * must be statically allocated.
 * Can be called at any time, even before retro_init(). */
RETRO_API void retro_get_system_info(struct retro_system_info *info);

/* Gets information about system audio/video timings and geometry.
 * Can be called only after retro_load_game() has successfully completed.
 * NOTE: The implementation of this function might not initialize every
 * variable if needed.
 * E.g. geom.aspect_ratio might not be initialized if core doesn't
 * desire a particular aspect ratio. */
RETRO_API void retro_get_system_av_info(struct retro_system_av_info *info);

/* Sets device to be used for player 'port'.
 * By default, RETRO_DEVICE_JOYPAD is assumed to be plugged into all
 * available ports.
 * Setting a particular device type is not a guarantee that libretro cores
 * will only poll input based on that particular device type. It is only a
 * hint to the libretro core when a core cannot automatically detect the
 * appropriate input device type on its own. It is also relevant when a
 * core can change its behavior depending on device type.
 *
 * As part of the core's implementation of retro_set_controller_port_device,
 * the core should call RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS to notify the
 * frontend if the descriptions for any controls have changed as a
 * result of changing the device type.
 */
RETRO_API void retro_set_controller_port_device(unsigned port, unsigned device);

/* Resets the current game. */
RETRO_API void retro_reset(void);

/* Runs the game for one video frame.
 * During retro_run(), input_poll callback must be called at least once.
 *
 * If a frame is not rendered for reasons where a game "dropped" a frame,
 * this still counts as a frame, and retro_run() should explicitly dupe
 * a frame if GET_CAN_DUPE returns true.
 * In this case, the video callback can take a NULL argument for data.
 */
RETRO_API void retro_run(void);

/* Returns the amount of data the implementation requires to serialize
 * internal state (save states).
 * Between calls to retro_load_game() and retro_unload_game(), the
 * returned size is never allowed to be larger than a previous returned
 * value, to ensure that the frontend can allocate a save state buffer once.
 */
RETRO_API size_t retro_serialize_size(void);

/* Serializes internal state. If failed, or size is lower than
 * retro_serialize_size(), it should return false, true otherwise. */
RETRO_API bool retro_serialize(void *data, size_t size);
RETRO_API bool retro_unserialize(const void *data, size_t size);

RETRO_API void retro_cheat_reset(void);
RETRO_API void retro_cheat_set(unsigned index, bool enabled, const char *code);

/* Loads a game.
 * Return true to indicate successful loading and false to indicate load failure.
 */
RETRO_API bool retro_load_game(const struct retro_game_info *game);

/* Loads a "special" kind of game. Should not be used,
 * except in extreme cases. */
RETRO_API bool retro_load_game_special(
  unsigned game_type,
  const struct retro_game_info *info, size_t num_info
);

/* Unloads the currently loaded game. Called before retro_deinit(void). */
RETRO_API void retro_unload_game(void);

/* Gets region of game. */
RETRO_API unsigned retro_get_region(void);

/* Gets region of memory. */
RETRO_API void *retro_get_memory_data(unsigned id);
RETRO_API size_t retro_get_memory_size(unsigned id);
