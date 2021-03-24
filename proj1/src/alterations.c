/* Name, alterations.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */

#include "alterations.h"

#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <assert.h>

#define NO_CHANGE                (0)
#define VLQ_MAX                  (128)
#define CHANGED                  (1)
#define NO_AVAILABLE_CHANNEL     (-1)

/* change_event_octave() will take a event_t type pointer and a
 * integer pointer of octave difference, and will change the
 * octave of the given event.
 */

int change_event_octave(event_t *original, int *octave) {
  if ((original == NULL) || (octave == NULL)) {
    return NO_CHANGE;
  }
  if (event_type(original) != MIDI_EVENT_T) {
    return NO_CHANGE;
  }
  if ((strcmp(original->midi_event.name, "Note Off") != 0) &&
  (strcmp(original->midi_event.name, "Note On") != 0) &&
  (strcmp(original->midi_event.name, "Polyphonic Key") != 0)) {
    return NO_CHANGE;
  }
  //printf("%d : %d\n", original->midi_event.data[0], *octave);
  if (((original->midi_event.data[0] + (*octave) * 12) < 0) ||
  ((original->midi_event.data[0] + (*octave) * 12) > 127)) {
    return NO_CHANGE;
  }
  original->midi_event.data[0] += (*octave) * 12;
  return CHANGED;
} /* change_event_octave() */

/* change_event_time will take a pointer of event_t struct and
 * a pointer of float, and will change the delta time duration
 * based on the float index given. It will return the difference
 * of byte the delta time occupies afterward.
 */

int change_event_time(event_t *original, float *input) {
  if ((original == NULL) || (input == NULL)) {
    return NO_CHANGE;
  }
  int orig_len = 0;
  if (original->delta_time < VLQ_MAX) {
    orig_len = 1;
  }
  else if (original->delta_time < (VLQ_MAX * VLQ_MAX)) {
    orig_len = 2;
  }
  else if (original->delta_time < (VLQ_MAX * VLQ_MAX * VLQ_MAX)) {
    orig_len = 3;
  }
  else {
    orig_len = 4;
  }
  if (original->delta_time * (*input) >= 0x0FFFFFFF) {
    original->delta_time = 0x0FFFFFF;
  }
  else {
    original->delta_time *= (*input);
  }

  int new_len = 0;
  if (original->delta_time < VLQ_MAX) {
    new_len = 1;
  }
  else if (original->delta_time < (VLQ_MAX * VLQ_MAX)) {
    new_len = 2;
  }
  else if (original->delta_time < (VLQ_MAX * VLQ_MAX * VLQ_MAX)) {
    new_len = 3;
  }
  else {
    new_len = 4;
  }
  return new_len - orig_len;
} /* change_event_time() */

/* change_event_instrument() will take an event_t pointer type
 * and remapping_t type array as parameters, and will change
 * the instrument used in the given event based on the remap
 * directed by the given remapping_t table.
 */

int change_event_instrument(event_t *original, remapping_t remap) {
  if (original == NULL) {
    return NO_CHANGE;
  }
  if (event_type(original) != MIDI_EVENT_T) {
    return NO_CHANGE;
  }
  if (strcmp(original->midi_event.name, "Program Change") != 0) {
    return NO_CHANGE;
  }
  original->midi_event.data = &(remap[(int)(original->midi_event.data[0])]);
  return CHANGED;
} /* change_event_instrument() */

/* change_event_note() takes a event_t pointer and a remapping_t
 * array as parameters, and will change the event's note value
 * using the index of remapping_t parameter
 */

int change_event_note(event_t *original, remapping_t remap) {
  if (original == NULL) {
    return NO_CHANGE;
  }
  if (event_type(original) != MIDI_EVENT_T) {
    return NO_CHANGE;
  }
  if (((original->midi_event.status & 0xF0) != 0x80) &&
  ((original->midi_event.status & 0xF0) != 0x90) &&
  ((original->midi_event.status & 0xF0) != 0xA0)) {
    return NO_CHANGE;
  }
  uint8_t new_data = remap[(int)original->midi_event.data[0]];
  original->midi_event.data[0] = new_data;
  return CHANGED;
} /* change_event_note() */

/* apply_to_events() will take a song_data_t type pointer, a
 * function pointer and a void arbitrary parameter. It will
 * read in all the events in the given song, and will apply
 * them to the given functions with the given arbitrary
 * parameter
 */

int apply_to_events(song_data_t *music, event_func_t
function_t, void *arbitrary) {
  if (music == NULL) {
    return NO_CHANGE;
  }
  track_node_t *current_track = music->track_list;
  int counter = 0;
  while (current_track != NULL) {
    event_node_t *current_event =
    current_track->track->event_list;
    while (current_event != NULL) {
      counter += function_t(current_event->event, arbitrary);
      current_event = current_event->next_event;
    }
    current_track = current_track->next_track;
  }
  return counter;
} /* apply_to_events() */


/* proxy_octave() will take a event_t pointer and a void change. This
 * function just serves as a proxy function for change_octave()
 */

int proxy_octave(event_t *original_event, void *change) {
  return change_event_octave(original_event, (int*)change);
} /* proxy_octave() */

/* change_octave will take a song_data_t type pointer and a int
 * of the actual octave change. It will change all the octaves
 * of the given songs(all events) using apply_to_events()
 */

int change_octave(song_data_t *original_song, int change) {
  if (original_song == NULL) {
    return NO_CHANGE;
  }
  return apply_to_events(original_song, &(proxy_octave), &change);
} /* change_octave() */

/* warp_time() will take a song_data_t type pointer and a float
 * time value, and will change the delta time of all events
 * in the given song as well as changing the corresponding
 * length of tracks after manupulation
 */

int warp_time(song_data_t *original_song, float time) {
  if (original_song == NULL) {
    return NO_CHANGE;
  }
  track_node_t *current_track = original_song->track_list;
  int counter = 0;
  while (current_track != NULL) {
    int modify = 0;
    event_node_t *current_event =
    current_track->track->event_list;
    while (current_event != NULL) {
      modify += change_event_time(current_event->event, &time);
      current_event = current_event->next_event;
    }
    current_track->track->length += modify;
    counter += modify;
    current_track = current_track->next_track;
  }
  return counter;
} /* warp_time() */

/* proxy_remap will take a event_t type pointer and a void remap
 * parameter. It simply serves as a proxy function for the
 * remap_instruments function
 */

int proxy_remap(event_t *original, void* remap) {
  return change_event_instrument(original, (uint8_t *) remap);
} /* proxy_remap() */

/* remap_instruments() will take a song_data_t type pointer and
 * a remapping_t array as parameter, adn will remap all the
 * instruments of each events in the song according to the remap
 * table. It uses apply_to_events() to accomplish that
 */

int remap_instruments(song_data_t *original, remapping_t remap) {
  if (original == NULL) {
    return NO_CHANGE;
  }
  return apply_to_events(original, &(proxy_remap), remap);
} /* remap_instruments() */

/* proxy_note() will take a event_t type pointer and a void
 * parameter. It serves as a proxy function for remap_notes()
 */

int proxy_note(event_t *original, void *remap) {
  return change_event_note(original, (uint8_t *) remap);
} /* proxy_note() */

/* remap_notes() will take a song_data_t type pointer and a
 * remapping_t type array, and will remap the song's note number
 * according to the given remap array
 */

int remap_notes(song_data_t *original, remapping_t remap) {
  if (original == NULL) {
    return NO_CHANGE;
  }
  return apply_to_events(original, &(proxy_note), remap);
} /* remap_notes() */

/* best_space() is a helper function that will take a pointer of
 * song_data_t struct. It will find out and return the best c
 * hannel for the new track to use. If all midi channel is used,
 * it wil return NO_AVAILABLE_CHANNEL;
 */

int best_space(song_data_t *original) {
  int channels[16] = {0};
  track_node_t *root_track = original->track_list;
  while (root_track != NULL) {
    event_node_t *root_event = root_track->track->event_list;
    while (root_event != NULL) {
      event_t *curr_event = root_event->event;
      if (event_type(curr_event) == MIDI_EVENT_T) {
        channels[(curr_event->midi_event.status) & 0x0F] = 1;
      }
      root_event = root_event->next_event;
    }
    root_track = root_track->next_track;
  }
  for (int i = 0; i < 16; i++) {
    if (channels[i] == 0) {
      return i;
    }
  }
  return NO_AVAILABLE_CHANNEL;
} /* best_space() */

/* deep_copy_event() is a helper function that will take a event_t
 * type pointer as parameter, and will return an deep copy of the event
 */

event_t *deep_copy_event(event_t *original) {
  event_t *output = malloc(sizeof(event_t));
  output->delta_time = original->delta_time;
  output->type = original->type;
  if (event_type(original) == META_EVENT_T) {
    meta_event_t new_meta = {original->meta_event.name,
    original->meta_event.data_len, NULL};
    if (new_meta.data_len != 0) {
      new_meta.data = malloc(original->meta_event.data_len);
      for (int i = 0; i < new_meta.data_len; i++) {
        new_meta.data[i] = original->meta_event.data[i];
      }
    }
    output->meta_event = new_meta;
  }
  else if (event_type(original) == SYS_EVENT_T) {
    sys_event_t output_sys = {0};
    output_sys.data_len = original->sys_event.data_len;
    if (output_sys.data_len != 0) {
      output_sys.data = malloc(output_sys.data_len);
      for (int i = 0; i < output_sys.data_len; i++) {
        output_sys.data[i] = original->sys_event.data[i];
      }
    }
    output->sys_event = output_sys;
  }
  else if (event_type(original) == MIDI_EVENT_T) {
    midi_event_t output_midi = {original->midi_event.name,
    original->midi_event.status, original->midi_event.data_len, NULL};
    if (output_midi.data_len != 0) {
      output_midi.data = malloc(output_midi.data_len);
      for (int i = 0; i < output_midi.data_len; i++) {
        output_midi.data[i] = original->midi_event.data[i];
      }
    }
    output->midi_event = output_midi;
    output->type = output_midi.status;
  }
  else {
    assert(1 == 0);
  }
  return output;
} /* deep_copy_event() */

/* deep_copy_track() will take a track_node_t type pointer as parameter
 * and will return a deep copy of this given track. The events inside
 * will also be deep-copied
 */

track_node_t *deep_copy_track(track_node_t *original) {
  track_node_t *output = malloc(sizeof(track_node_t));
  output->next_track = NULL;
  output->track = malloc(sizeof(track_t));
  output->track->length = original->track->length;
  event_node_t *current_event = original->track->event_list;
  event_node_t *new_event_node = malloc(sizeof(event_node_t));
  output->track->event_list = new_event_node;
  new_event_node->event = deep_copy_event(current_event->event);
  current_event = current_event->next_event;
  while (current_event != NULL) {
    event_node_t *new_next_event = malloc(sizeof(event_node_t));
    new_next_event->event = deep_copy_event(current_event->event);
    new_next_event->next_event = NULL;
    new_event_node->next_event = new_next_event;
    new_event_node = new_event_node->next_event;
    current_event = current_event->next_event;
  }
  new_event_node->next_event = NULL;
  return output;
} /* deep_copy_track() */

/* add_round will take the original song, a index for a specific track,
 * an octave change integer, a unsigned integer of time delay and a
 * uint8_t instrument specifier, and will deep-copy the specific track
 * given by the track index, change all of its octave by the given octave
 * change. delay it by the specific time_delay and re-assign its
 * instrument to be the specific instrument. It will add the new track
 * at the end of the song and update all the information of the song
 */

void add_round(song_data_t *original, int track_index, int octave_dif,
unsigned int time_delay, uint8_t instrument) {
  assert(original != NULL);
  assert(original->num_tracks > track_index);
  assert(original->format != 2);
  track_node_t* current_track = original->track_list;
  int available_channel = best_space(original);
  assert(available_channel != -1);
  for (int i = 0; i < track_index; i++) {
    current_track = current_track->next_track;
    printf("%s: %d\n", "We're taking", i);
  }
  track_node_t *new_track_t = deep_copy_track(current_track);
  new_track_t->next_track = NULL;
  event_node_t *current_event_node = new_track_t->track->event_list;
  int orig_len = 0;
  if (current_event_node->event->delta_time < VLQ_MAX) {
    orig_len = 1;
  }
  else if (current_event_node->event->delta_time < (VLQ_MAX * VLQ_MAX)) {
    orig_len = 2;
  }
  else if (current_event_node->event->delta_time <
  (VLQ_MAX * VLQ_MAX * VLQ_MAX)) {
    orig_len = 3;
  }
  else {
    orig_len = 4;
  }
  int new_len = 0;
  current_event_node->event->delta_time += time_delay;
  if (current_event_node->event->delta_time < VLQ_MAX) {
    new_len = 1;
  }
  else if (current_event_node->event->delta_time < (VLQ_MAX * VLQ_MAX)) {
    new_len = 2;
  }
  else if (current_event_node->event->delta_time <
  (VLQ_MAX * VLQ_MAX * VLQ_MAX)) {
    new_len = 3;
  }
  else {
    new_len = 4;
  }
  new_track_t->track->length += (new_len - orig_len);
  while (current_event_node != NULL) {
    change_event_octave(current_event_node->event, &octave_dif);
    if (event_type(current_event_node->event) == MIDI_EVENT_T) {
      if (strcmp(current_event_node->event->midi_event.name,
      "Program Change") == 0) {
        current_event_node->event->midi_event.data[0] = instrument;
      }
    }
    current_event_node = current_event_node->next_event;
  }
  while (current_track->next_track != NULL) {
    current_track = current_track->next_track;
  }
  current_track->next_track = new_track_t;
  current_track = original->track_list;
  current_event_node = new_track_t->track->event_list;
  original->format = 1;
  original->num_tracks += 1;
  while (current_event_node != NULL) {
    if (event_type(current_event_node->event) == MIDI_EVENT_T) {
      current_event_node->event->midi_event.status =
      (current_event_node->event->midi_event.status & 0b11110000) |
      available_channel;
      current_event_node->event->type =
      current_event_node->event->midi_event.status;
    }
    current_event_node = current_event_node->next_event;
  }
} /* add_round() */

/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables()
{
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i - 1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i + 1;
  }
} /* build_mapping_tables() */
