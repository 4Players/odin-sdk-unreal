/* Copyright (c) 4Players GmbH. All rights reserved. */

#pragma once

/** @file */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ODIN_VERSION "1.6.7"

/**
 * Known types of a media stream.
 *
 * Note: Video streams are not supported yet.
 */
typedef enum OdinMediaStreamType {
    /**
     * Media stream is of type audio
     */
    OdinMediaStreamType_Audio,
    /**
     * Media stream is of type video
     */
    OdinMediaStreamType_Video,
    /**
     * Media stream is invalid
     */
    OdinMediaStreamType_Invalid,
} OdinMediaStreamType;

/**
 * Valid levels for aggressiveness of the noise suppression. A higher level will reduce the noise
 * level at the expense of a higher speech distortion.
 */
typedef enum OdinNoiseSuppressionLevel {
    /**
     * Noise suppression is disabled
     */
    OdinNoiseSuppressionLevel_None,
    /**
     * Use low suppression (6 dB)
     */
    OdinNoiseSuppressionLevel_Low,
    /**
     * Use moderate suppression (12 dB)
     */
    OdinNoiseSuppressionLevel_Moderate,
    /**
     * Use high suppression (18 dB)
     */
    OdinNoiseSuppressionLevel_High,
    /**
     * Use very high suppression (21 dB)
     */
    OdinNoiseSuppressionLevel_VeryHigh,
} OdinNoiseSuppressionLevel;

/**
 * All valid connection states for an ODIN room.
 */
typedef enum OdinRoomConnectionState {
    /**
     * Connection is being established
     */
    OdinRoomConnectionState_Connecting,
    /**
     * Connection is established
     */
    OdinRoomConnectionState_Connected,
    /**
     * Connection is being closed
     */
    OdinRoomConnectionState_Disconnecting,
    /**
     * Connection is closed
     */
    OdinRoomConnectionState_Disconnected,
} OdinRoomConnectionState;

/**
 * Possible reasons for connection state changes of an ODIN room.
 */
typedef enum OdinRoomConnectionStateChangeReason {
    /**
     * Connection state change was initiated by the user
     */
    OdinRoomConnectionStateChangeReason_ClientRequested,
    /**
     * Connection state change was initiated by the server (e.g. peer was kicked)
     */
    OdinRoomConnectionStateChangeReason_ServerRequested,
    /**
     * Connection state change was caused by a timeout
     */
    OdinRoomConnectionStateChangeReason_ConnectionLost,
} OdinRoomConnectionStateChangeReason;

/**
 * Valid audiences for ODIN room tokens.
 */
typedef enum OdinTokenAudience {
    /**
     * JWT has no audience
     */
    OdinTokenAudience_None,
    /**
     * JWT is accepted the ODIN gateway
     */
    OdinTokenAudience_Gateway,
    /**
     * JWT is accepted by the ODIN server
     */
    OdinTokenAudience_Sfu,
} OdinTokenAudience;

/**
 * A pointer to a local ODIN token generator, employed for generating signed room tokens predicated
 * on an access key. Be aware that access keys serve as your unique authentication keys, requisite
 * for generating room tokens to access the ODIN server network. To ensure your security, it's
 * strongly recommended that you _NEVER_ embed an access key within your client code, and instead
 * generate room tokens on a server.
 */
typedef struct OdinTokenGenerator OdinTokenGenerator;

/**
 * A numeric code returned by ODIN function calls. Use `odin_is_error` to determine whether the
 * code represents an error or an actual result value.
 *
 * Note: Use `odin_error_format` to get a human readable string to represent error codes.
 */
typedef uint32_t OdinReturnCode;

/**
 * Audio stream configuration.
 */
typedef struct OdinAudioStreamConfig {
    /**
     * The number of samples per second in hertz (between 8000 and 192000)
     */
    uint32_t sample_rate;
    /**
     * The number of channels for the new audio stream (between 1 and 2)
     */
    uint8_t channel_count;
} OdinAudioStreamConfig;

/**
 * Internal handle identifier for an ODIN room to interact with.
 */
typedef size_t OdinRoomHandle;

/**
 * Internal handle identifier for an ODIN audio/video stream used to send/receive data.
 */
typedef size_t OdinMediaStreamHandle;

/**
 * All the different events emitted from an ODIN room.
 */
typedef enum OdinEventTag {
    /**
     * Emitted after joining once initial room information was processed
     */
    OdinEvent_Joined,
    /**
     * Emitted when other peers joined the room
     */
    OdinEvent_PeerJoined,
    /**
     * Emitted when other peers left the room
     */
    OdinEvent_PeerLeft,
    /**
     * Emitted when other peers updated their user data
     */
    OdinEvent_PeerUserDataChanged,
    /**
     * Emitted when other peers started a media stream
     */
    OdinEvent_MediaAdded,
    /**
     * Emitted when other peers stopped a media stream
     */
    OdinEvent_MediaRemoved,
    /**
     * Emitted whenever data is sent/received over any known media
     */
    OdinEvent_MediaActiveStateChanged,
    /**
     * Emitted when other peers changed the global user data of the room itself
     */
    OdinEvent_RoomUserDataChanged,
    /**
     * Emitted when the internal room connection state of the ODIN client changed
     */
    OdinEvent_RoomConnectionStateChanged,
    /**
     * Emitted when others peers sent arbitrary data
     */
    OdinEvent_MessageReceived,
} OdinEventTag;

typedef struct OdinEvent_JoinedData {
    /**
     * Name of the joined room (null-terminated)
     */
    const char *room_id;
    /**
     * Length of the room name
     */
    size_t room_id_len;
    /**
     * Byte array with arbitrary user data of the room
     */
    const uint8_t *room_user_data;
    /**
     * Length of the room user data array
     */
    size_t room_user_data_len;
    /**
     * Identifier of the customer the room is assigned to (nul-terminated)
     */
    const char *customer;
    /**
     * Length of the customer identifier
     */
    size_t customer_len;
    /**
     * Own peer ID assigned by the server
     */
    uint64_t own_peer_id;
    /**
     * Own user identifier of the peer specified during authentication (null-terminated)
     */
    const char *own_user_id;
    /**
     * Length of the own user identifier
     */
    size_t own_user_id_len;
} OdinEvent_JoinedData;

typedef struct OdinEvent_PeerJoinedData {
    /**
     * ID of the peer
     */
    uint64_t peer_id;
    /**
     * Byte array with arbitrary user data of the peer
     */
    const uint8_t *peer_user_data;
    /**
     * Length of the room user data array
     */
    size_t peer_user_data_len;
    /**
     * User identifier of the peer specified during authentication (null-terminated)
     */
    const char *user_id;
    /**
     * Length of the user identifier
     */
    size_t user_id_len;
} OdinEvent_PeerJoinedData;

typedef struct OdinEvent_PeerLeftData {
    /**
     * ID of the peer
     */
    uint64_t peer_id;
} OdinEvent_PeerLeftData;

typedef struct OdinEvent_PeerUserDataChangedData {
    /**
     * ID of the peer
     */
    uint64_t peer_id;
    /**
     * Byte array with arbitrary user data of the peer
     */
    const uint8_t *peer_user_data;
    /**
     * Length of the room user data array
     */
    size_t peer_user_data_len;
} OdinEvent_PeerUserDataChangedData;

typedef struct OdinEvent_MediaAddedData {
    /**
     * ID of the peer this media belongs to
     */
    uint64_t peer_id;
    /**
     * Handle identifier of the new audio/video stream
     */
    OdinMediaStreamHandle media_handle;
} OdinEvent_MediaAddedData;

typedef struct OdinEvent_MediaRemovedData {
    /**
     * ID of the peer this media belongs to
     */
    uint64_t peer_id;
    /**
     * Handle identifier of the audio/video stream
     */
    OdinMediaStreamHandle media_handle;
} OdinEvent_MediaRemovedData;

typedef struct OdinEvent_MediaActiveStateChangedData {
    /**
     * ID of the peer this media belongs to
     */
    uint64_t peer_id;
    /**
     * Handle identifier of the audio/video stream
     */
    OdinMediaStreamHandle media_handle;
    /**
     * Indicator for whether or not the media is sending/receiving data
     */
    bool active;
} OdinEvent_MediaActiveStateChangedData;

typedef struct OdinEvent_RoomUserDataChangedData {
    /**
     * Byte array with arbitrary user data of the room
     */
    const uint8_t *room_user_data;
    /**
     * Length of the room user data array
     */
    size_t room_user_data_len;
} OdinEvent_RoomUserDataChangedData;

typedef struct OdinEvent_RoomConnectionStateChangedData {
    /**
     * Status of the room connection
     */
    enum OdinRoomConnectionState state;
    /**
     * Reason for this update
     */
    enum OdinRoomConnectionStateChangeReason reason;
} OdinEvent_RoomConnectionStateChangedData;

typedef struct OdinEvent_MessageReceivedData {
    /**
     * ID of the peer who sent this message
     */
    uint64_t peer_id;
    /**
     * Byte array with arbitrary data received
     */
    const uint8_t *data;
    /**
     * Length of the data array
     */
    size_t data_len;
} OdinEvent_MessageReceivedData;

typedef struct OdinEvent {
    OdinEventTag tag;
    union {
        OdinEvent_JoinedData joined;
        OdinEvent_PeerJoinedData peer_joined;
        OdinEvent_PeerLeftData peer_left;
        OdinEvent_PeerUserDataChangedData peer_user_data_changed;
        OdinEvent_MediaAddedData media_added;
        OdinEvent_MediaRemovedData media_removed;
        OdinEvent_MediaActiveStateChangedData media_active_state_changed;
        OdinEvent_RoomUserDataChangedData room_user_data_changed;
        OdinEvent_RoomConnectionStateChangedData room_connection_state_changed;
        OdinEvent_MessageReceivedData message_received;
    };
} OdinEvent;

/**
 * Statistics for the underlying connection of a room.
 */
typedef struct OdinConnectionStats {
    /**
     * The amount of outgoing UDP datagrams observed
     */
    uint64_t udp_tx_datagrams;
    /**
     * The amount of outgoing acknowledgement frames observed
     */
    uint64_t udp_tx_acks;
    /**
     * The total amount of bytes which have been transferred inside outgoing UDP datagrams
     */
    uint64_t udp_tx_bytes;
    /**
     * The amount of incoming UDP datagrams observed
     */
    uint64_t udp_rx_datagrams;
    /**
     * The amount of incoming acknowledgement frames observed
     */
    uint64_t udp_rx_acks;
    /**
     * The total amount of bytes which have been transferred inside incoming UDP datagrams
     */
    uint64_t udp_rx_bytes;
    /**
     * Current congestion window of the connection
     */
    uint64_t cwnd;
    /**
     * Congestion events on the connection
     */
    uint64_t congestion_events;
    /**
     * Current best estimate of the connection latency (round-trip-time) in milliseconds
     */
    float rtt;
} OdinConnectionStats;

/**
 * Per-room configuration of the ODIN audio processing module which provides a variety of smart
 * enhancement algorithms.
 */
typedef struct OdinApmConfig {
    /**
     * Enables or disables voice activity detection (VAD)
     */
    bool voice_activity_detection;
    /**
     * Voice probability value when the VAD should engage
     */
    float voice_activity_detection_attack_probability;
    /**
     * Voice probability value when the VAD should disengage
     */
    float voice_activity_detection_release_probability;
    /**
     * Enables or disables the input volume gate
     */
    bool volume_gate;
    /**
     * Root mean square power (dBFS) when the volume gate should engage
     */
    float volume_gate_attack_loudness;
    /**
     * Root mean square power (dBFS) when the volume gate should disengage
     */
    float volume_gate_release_loudness;
    /**
     * Enable or disable echo cancellation
     */
    bool echo_canceller;
    /**
     * Enable or disable high pass filtering
     */
    bool high_pass_filter;
    /**
     * Enable or disable the pre amplifier
     */
    bool pre_amplifier;
    /**
     * Set the aggressiveness of the suppression
     */
    enum OdinNoiseSuppressionLevel noise_suppression_level;
    /**
     * Enable or disable the transient suppressor
     */
    bool transient_suppressor;
    /**
     * Enable or disable the gain controller
     */
    bool gain_controller;
} OdinApmConfig;

/**
 * Audio stream statistics.
 */
typedef struct OdinAudioStreamStats {
    /**
     * The total number of packets seen by the medias jitter buffer.
     */
    uint32_t packets_total;
    /**
     * The number of packets processed by the medias jitter buffer.
     */
    uint32_t packets_processed;
    /**
     * The number of packets dropped because they seemed to arrive too early.
     */
    uint32_t packets_arrived_too_early;
    /**
     * The number of packets dropped because they seemed to arrive too late.
     */
    uint32_t packets_arrived_too_late;
    /**
     * The number of packets dropped due to a jitter buffer reset.
     */
    uint32_t packets_dropped;
    /**
     * The number of packets marked as invalid.
     */
    uint32_t packets_invalid;
    /**
     * The number of packets marked as duplicates.
     */
    uint32_t packets_repeated;
    /**
     * The number of packets marked as lost during transmission.
     */
    uint32_t packets_lost;
} OdinAudioStreamStats;

typedef size_t OdinResamplerHandle;

/**
 * Options for ODIN room tokens.
 */
typedef struct OdinTokenOptions {
    /**
     * Customer identifier (you should _NOT_ set this unless connecting directly to an ODIN server)
     */
    const char *customer;
    /**
     * Audience of the token
     */
    enum OdinTokenAudience audience;
    /**
     * Token lifetime in seconds
     */
    uint64_t lifetime;
} OdinTokenOptions;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * Formats an ODIN return code into a human readable string representation for use in logging and
 * diagnostics. If `buf` is `NULL` this functions simply returns the required buffer length to
 * store the output buffer.
 */
size_t odin_error_format(OdinReturnCode error, char *buf, size_t buf_len);

/**
 * Checks whether the code returned from ODIN function calls represents an error or an actual
 * result. This is used to easier work with certain functions that might return an error or a
 * valid result like `odin_audio_data_len`.
 * Internally this simply does `(code >> 29) > 0`.
 */
bool odin_is_error(OdinReturnCode code);

/**
 * Initializes the internal ODIN client runtime with optimized settings for audio output, ensuring
 * the correct API header file is employed. This operation is ref-counted, necessitating paired
 * invocations of startup and shutdown within your application. The majority of the API functions
 * hinge on an active ODIN runtime, with the sole exception of `access_key` and `token_generator`
 * related functions.
 *
 * Note: Utilize `ODIN_VERSION` to supply the `version` argument.
 */
bool odin_startup(const char *version);

/**
 * Initializes the internal ODIN client runtime, permitting the specification of sample rate and
 * channel layout for audio output. This operation is ref-counted, necessitating paired invocations
 * of startup and shutdown within your application.
 *
 * Note: Ensure consistent settings are used on successive invocations of this function.
 */
bool odin_startup_ex(const char *version, struct OdinAudioStreamConfig output_config);

/**
 * Shuts down the internal ODIN runtime. It is advisable to invoke this function prior to
 * terminating the application. Post invocation, all `odin_*` methods will cease to function
 * immediately, provided the internal ref-count has descended to zero. Refer to `odin_startup`
 * for additional details.
 */
void odin_shutdown(void);

/**
 * Creates a new ODIN room handle in an unconnected state and returns its handle identifier. This
 * will return `0` when the internal ODIN client runtime is not initialized using `odin_startup`
 * or has already been terminated using `odin_shutdown`.
 */
OdinRoomHandle odin_room_create(void);

/**
 * Closes the specified ODIN room handle, thus making our own peer leave the room on the server
 * and closing the connection if needed.
 */
OdinReturnCode odin_room_close(OdinRoomHandle room);

/**
 * Destroys the specified ODIN room handle.
 */
OdinReturnCode odin_room_destroy(OdinRoomHandle room);

/**
 * Sets the event callback on the the specified `OdinRoomHandle`. Generally this should be called
 * _once_ before joining a room.
 */
OdinReturnCode odin_room_set_event_callback(OdinRoomHandle room,
                                            void (*callback)(OdinRoomHandle room,
                                                             const struct OdinEvent *event,
                                                             void *extra_data),
                                            void *extra_data);

/**
 * Sets the scaling factor for coordinates supplied to `odin_room_update_position`, facilitating
 * adaptation to your game's unique coordinate system requirements. Peers are visible to each other
 * only within a unit circle of radius `1.0`. When altering a peer's position, ensure the position
 * is scaled such that the maximum distance remains one or less. This scaling can be performed
 * manually or by specifying the multiplicative scale here.
 *
 * Note: It's crucial to maintain consistent scaling across all client applications.
 */
OdinReturnCode odin_room_set_position_scale(OdinRoomHandle room, float scale);

/**
 * Joins a room on an ODIN server. This function takes an URL to an ODIN gateway and a signed room
 * token obtained externally that authorizes the client to establish the connection. Unless you're
 * hosting your own servers, always use gateway running at `https://gateway.odin.4players.io`.
 */
OdinReturnCode odin_room_join(OdinRoomHandle room, const char *url, const char *token);

/**
 * Retrieves the room ID (e.g. the name of the room) from the specified `OdinRoomHandle`.
 */
OdinReturnCode odin_room_id(OdinRoomHandle room, char *out_id, size_t out_id_len);

/**
 * Retrieves the identifier of the customer the room is assigned to from the specified `OdinRoomHandle`.
 */
OdinReturnCode odin_room_customer(OdinRoomHandle room,
                                  char *out_customer,
                                  size_t out_customer_len);

/**
 * Retrieves your own peer ID from the specified `OdinRoomHandle`.
 */
OdinReturnCode odin_room_peer_id(OdinRoomHandle room, uint64_t *out_peer_id);

/**
 * Retrieves statistics for the underlying connection of the specified `OdinRoomHandle`.
 */
OdinReturnCode odin_room_connection_stats(OdinRoomHandle room, struct OdinConnectionStats *stats);

/**
 * Updates the custom user data for your own peer. All user data is synced automatically, which
 * allows storing of arbitrary information for each individual peer.
 *
 * Note: Use this before calling `odin_room_join` to set initial peer user data upon connect.
 */
OdinReturnCode odin_room_update_peer_user_data(OdinRoomHandle room,
                                               const uint8_t *user_data,
                                               size_t user_data_length);

/**
 * Updates the three-dimensional position of the current peer within the specified `OdinRoomHandle`.
 * The server utilizes the provided coordinates to perform automatic culling among peers in the same
 * room, based on unit circles with a radius of `1.0`. This feature is particularly beneficial in
 * scenarios involving a large number of peers within the same room, enabling peers to interact or
 * 'see' each other only when in close proximity. To modify the distance sensitivity for position
 * updates, use `odin_room_set_position_scale`.
 *
 * Note: Use this before calling `odin_room_join` to set the initial peer position upon connect.
 */
OdinReturnCode odin_room_update_position(OdinRoomHandle room, float x, float y, float z);

/**
 * Sends arbitrary data to a list of target peers over the ODIN server. If `NULL` is specified, the
 * message will be sent to all other peers in the same room.
 */
OdinReturnCode odin_room_send_message(OdinRoomHandle room,
                                      const uint64_t *peer_id_list,
                                      size_t peer_id_list_size,
                                      const uint8_t *data,
                                      size_t data_length);

/**
 * Adds a specified `OdinMediaStreamHandle` to the room. Please note, that this can only be done
 * _once_ on a given media. Trying to do it more than once will return an error on subsequent calls
 * to this function.
 */
OdinReturnCode odin_room_add_media(OdinRoomHandle room, OdinMediaStreamHandle media);

/**
 * Configures the ODIN audio processing module on the room with the specified config.
 */
OdinReturnCode odin_room_configure_apm(OdinRoomHandle room, struct OdinApmConfig config);

/**
 * Creates a new audio input stream, which can be added to a room and send data over it.
 */
OdinMediaStreamHandle odin_audio_stream_create(struct OdinAudioStreamConfig config);

/**
 * Creates a new video input stream, which can be added to a room and send data over it.
 *
 * Note: Video streams are not supported yet.
 */
OdinMediaStreamHandle odin_video_stream_create(void);

/**
 * Destroys the specified `OdinMediaStreamHandle`, after which you will no longer be able to
 * receive or send any data over it. If the media is currently 'attached' to a room it will be
 * removed.
 */
OdinReturnCode odin_media_stream_destroy(OdinMediaStreamHandle stream);

/**
 * Retrieves the media ID of the specified `OdinMediaStreamHandle`.
 */
OdinReturnCode odin_media_stream_media_id(OdinMediaStreamHandle stream, uint16_t *out_media_id);

/**
 * Retrieves the peer ID of the specified `OdinMediaStreamHandle`.
 */
OdinReturnCode odin_media_stream_peer_id(OdinMediaStreamHandle stream, uint64_t *out_peer_id);

/**
 * Returns the type of the specified media stream.
 *
 * Note: This function will always return `OdinMediaStreamType_Audio` at the moment.
 */
enum OdinMediaStreamType odin_media_stream_type(OdinMediaStreamHandle stream);

/**
 * Instructs the server to pause the specified `OdinMediaStreamHandle`, ceasing the reception of
 * data. This operation essentially communicates a server-side mute request from the client, thus
 * indicating a desire to halt packet reception for this media stream.
 */
OdinReturnCode odin_media_stream_pause(OdinMediaStreamHandle stream);

/**
 * Instructs the server to resume the specified output `OdinMediaStreamHandle`, re-initiating the
 * reception of data. This operation essentially communicates a server-side unmute request from the
 * client, indicating a desire to restart packet reception for this media stream.
 */
OdinReturnCode odin_media_stream_resume(OdinMediaStreamHandle stream);

/**
 * Sends data to the audio stream. The data has to be interleaved [-1, 1] float data.
 */
OdinReturnCode odin_audio_push_data(OdinMediaStreamHandle stream, const float *buf, size_t buf_len);

/**
 * Reads audio data from the specified `OdinMediaStreamHandle`. This will return audio data in the
 * format specified when calling `odin_startup_ex` or 48 kHz interleaved by default.
 */
OdinReturnCode odin_audio_read_data(OdinMediaStreamHandle stream,
                                    float *out_buffer,
                                    size_t out_buffer_len);

/**
 * Returns the number of samples available in the audio buffer of an output `OdinMediaStreamHandle`.
 */
OdinReturnCode odin_audio_data_len(OdinMediaStreamHandle stream);

/**
 * Resets the specified `OdinMediaStreamHandle` to its initial state, restoring it to its default
 * configuration. This operation resets the internal Opus encoder/decoder, ensuring a clean state.
 * Additionally, it clears internal buffers, providing a fresh start.
 */
OdinReturnCode odin_audio_reset(OdinMediaStreamHandle stream);

/**
 * Retrieves statistics for the specified `OdinMediaStreamHandle`.
 *
 * Note: This will only work for output streams.
 */
OdinReturnCode odin_audio_stats(OdinMediaStreamHandle stream, struct OdinAudioStreamStats *stats);

/**
 * Reads up to `out_buffer_len` samples from the given streams and mixes them into the `out_buffer`.
 * All audio streams will be read based on the sample rate you chose when initializing the ODIN runtime
 * so make sure to allocate the buffer accordingly. After the call the `out_buffer_len` will contain
 * the amount of samples that have actually been read and mixed into `out_buffer`.
 *
 * If enabled this will also apply any audio processing to the output stream and feed back required
 * data to the internal audio processing pipeline which requires a final mix.
 */
OdinReturnCode odin_audio_mix_streams(OdinRoomHandle room,
                                      const OdinMediaStreamHandle *streams,
                                      size_t stream_count,
                                      float *out_buffer,
                                      size_t out_buffer_len);

/**
 * Processes the reverse audio stream, also known as the loopback data to be used in the ODIN echo
 * canceller. This should only be done if you are _NOT_ using `odin_audio_mix_streams`.
 */
OdinReturnCode odin_audio_process_reverse(OdinRoomHandle room, float *buffer, size_t buffer_len);

/**
 * Sets the delay estimate for the reverse stream used in the ODIN echo cancellation. This function
 * is important in scenarios where the audio output and the audio input are not synchronized. An
 * accurate delay value ensures that the echo canceller can correctly align the two audio streams,
 * resulting in effective echo cancellation.
 *
 * Improper delay values may lead to poor echo cancellation and thus degrade the quality of the
 * audio communication.
 */
OdinReturnCode odin_audio_set_stream_delay(OdinRoomHandle room, uint64_t delay_ms);

/**
 * Creates a new ODIN resampler instance. This is intended for situations where your audio pipeline
 * doesn't support 48 kHz.
 *
 * Note: One resampler should be used exclusively per audio stream.
 */
OdinResamplerHandle odin_resampler_create(uint32_t from_rate,
                                          uint32_t to_rate,
                                          uint16_t channel_count);

/**
 * Resamples a single chunk of audio. If the ODIN resampler instance was created with multiple
 * channels, the data is assumed to be interleaved. The `output_capacity` argument also serves as
 * an out parameter when the provided capacity wasn't enough to fulfill the resample request, in
 * which case this function will write the minimum required buffer size into the given variable.
 * On success, the written size for the processed sample is returned in both, the return value
 * and the `output_capacity` out parameter.
 */
OdinReturnCode odin_resampler_process(OdinResamplerHandle resampler,
                                      const float *input,
                                      size_t input_len,
                                      float *output,
                                      size_t *output_capacity);

/**
 * Destroys the given ODIN resampler instance. After this call, all attempts to use this handle
 * will fail.
 */
OdinReturnCode odin_resampler_destroy(OdinResamplerHandle resampler);

/**
 * Creates a new access key crucial for signing tokens, facilitating access to an ODIN server. An
 * access key is a 44-character long Base64 String, embodying a version identifier, random bytes,
 * and a checksum.
 */
OdinReturnCode odin_access_key_generate(char *buf, size_t buf_len);

/**
 * Extracts the key ID from a specified access key. The key ID is embedded in room tokens, enabling
 * the identification of the corresponding public key required for verification.
 */
OdinReturnCode odin_access_key_id(const char *access_key, char *out_key_id, size_t out_key_id_len);

/**
 * Extracts the public key from a specified access key. The public key, derived from the Ed25519
 * curve, must be shared with _4Players_ to enable verification of a generated room token.
 */
OdinReturnCode odin_access_key_public_key(const char *access_key,
                                          char *out_public_key,
                                          size_t out_public_key_len);

/**
 * Extracts the private key from a specified access key. The private key, rooted in the Ed25519
 * curve, is utilized to sign a generated room token for accessing the ODIN network.
 */
OdinReturnCode odin_access_key_secret_key(const char *access_key,
                                          char *out_secret_key,
                                          size_t out_secret_key_len);

/**
 * Creates a new token generator instance.
 */
struct OdinTokenGenerator *odin_token_generator_create(const char *access_key);

/**
 * Destroys an existing token generator instance.
 */
void odin_token_generator_destroy(struct OdinTokenGenerator *generator);

/**
 * Generates a signed JWT, which can be used by an ODIN client to join a room.
 */
OdinReturnCode odin_token_generator_create_token(struct OdinTokenGenerator *generator,
                                                 const char *room_id,
                                                 const char *user_id,
                                                 char *out_token,
                                                 size_t out_token_len);

/**
 * Generates a signed JWT such as `odin_token_generator_create_token` and allows passing a custom
 * set of `OdinTokenOptions` for advanced use-cases.
 */
OdinReturnCode odin_token_generator_create_token_ex(struct OdinTokenGenerator *generator,
                                                    const char *room_id,
                                                    const char *user_id,
                                                    const struct OdinTokenOptions *options,
                                                    char *out_token,
                                                    size_t out_token_len);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
