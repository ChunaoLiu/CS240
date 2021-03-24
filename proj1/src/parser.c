/* Chunao Liu, parser.c, CS 24000, Fall 2020
 * Last updated November 7, 2020
 */

/* Add any includes here */

#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser.h"

#define SAME_STUFF              (1)
#define NO_SAME_STUFF           (-1)
#define IMPOSSIBLE              (-10000000)
#define NOT_WRITTEN             (-1)
#define WRITTEN                 (1)
#define DIV_MASKING_1           (0x01FE)
#define DIV_MASKING_2           (0xFE00)
#define HEAD_CHUNK              ("MThd")
#define TRACK_CHUNK             ("MTrk")

/* global variable/calls goes here */

uint8_t g_midi_statue = 0;

/* song_data_t will() take a file name as parameter,
 * and will read in a full song_data_t and return
 * its pointer. It's associated with a lot of other
 * functions.
 */

song_data_t *parse_file(const char *name) {
  assert(name != NULL);
  printf("%s: %s\n", "Reading", name);
  FILE *file = fopen(name, "r");
  assert(file != NULL);
  song_data_t *midi_song = 0;
  midi_song = malloc(sizeof(song_data_t));
  assert(midi_song != NULL);
  midi_song->path = malloc(strlen(name) + 1);
  assert(midi_song->path != NULL);
  strcpy(midi_song->path, name);
  parse_header(file, midi_song);
  parse_track(file, midi_song);
  assert(getc(file) == EOF);
  fclose(file);
  return midi_song;
} /* parse_file() */

/* parse_header will take a file pointer and a
 * song_data_t pointer, and will read in the
 * header information of a song and parse it
 * into the given song_data_t pointer. This
 * should usually be the first thing to do.
 */

  void parse_header(FILE *file, song_data_t *output) {
  assert(file != NULL);
  assert(output != NULL);
  char type[5] = {0};
  int check_return = fread(type, 1, 4, file);

  /* The first 4 byte should be "MThd, and if not, we're in
   * big trouble. That's why we need to assert it.
   */

  assert(check_return == 4);
  assert(strcmp(type, HEAD_CHUNK) == 0);
  uint8_t length_head[6] = {0};
  check_return = fread(length_head, 1, 4, file);
  assert(check_return == 4);
  uint16_t format_16 = 0;

  /* The length of the later header chunk should be
   * exactly 6 byte, but due to it is a 32 bit uint, we
   * will do something different here by comparing each
   * byte one by one so that we don't even need to shift
   * the endianness. My Laziness Wins Again
   */

  assert(length_head[0] == 0);
  assert(length_head[1] == 0);
  assert(length_head[2] == 0);
  assert(length_head[3] == 6);
  check_return = fread(length_head, 1, 2, file);
  assert(check_return == 2);

  /* We shift the endianness of the format just read in
   * by the following code, then we need to assert it to be
   * either 0, 1, or 2. If it is 0, then there can be only
   * one track (num_track == 1) so we need to assert that as well
   */

  format_16 = ((uint16_t)length_head[0] << 8) | ((uint16_t) length_head[1]);
  assert((format_16 >= 0) && (format_16 <= 2));
  assert(check_return == 2);
  output->format = format_16;
  check_return = fread(&(output->num_tracks), 2, 1, file);
  output->num_tracks = (output->num_tracks >> 8) |
  (output->num_tracks << 8);
  assert(check_return == 1);
  if (output->format == 0) {
    assert(output->num_tracks == 1);
  }

  /* Now we will read in the division and compare the last (first bit
   * If the endianness is not shift) bit to confirm the format it is
   * using. If it is one, then we will read first 7 bits as ticks per
   * frame and the remaining (except the one we used for comparison)
   * as frames per sec. If the last bit is 0, we'll read everything
   * except the last byte into ticks_per_tqp. They are stored in a
   * union so there will be no wast of memory space.
   */

  uint16_t division_temp = 0;
  check_return = fread(&division_temp, 2, 1, file);
  assert(check_return == 1);
  division_temp = (division_temp >> 8) | (division_temp << 8); 
  assert(check_return == 1);
  division_t output_div = {0};
  assert(((division_temp & 0x0001) == 0x0001) ||
  ((division_temp & 0x0001) == 0x0000));
  if ((division_temp & 0x0001) == 0x0001) {
    output_div.uses_tpq = false;
    output_div.ticks_per_frame = (uint8_t)(division_temp & DIV_MASKING_1);
    output_div.frames_per_sec = (uint8_t)((division_temp & DIV_MASKING_2) >> 8);
  }
  else if ((division_temp & 0x0001) == 0x0000) {
    output_div.uses_tpq = true;
    output_div.ticks_per_qtr = (division_temp & 0b1111111111111110);
  }
  output->division = output_div;
} /* parse_header() */

/* parse_track will take in a file pointer and a song_data_t type
 * pointer. It will read the tracks and make it into a linked list
 * while assign the head of the list to the song_data_t's track
 */

void parse_track(FILE *file, song_data_t *output) {
  track_node_t *track_root = 0;
  track_root = malloc(sizeof(track_node_t));
  assert(track_root != NULL);
  track_node_t *head_track = track_root;
  int flag = NOT_WRITTEN;
  for (int i = 0; i < output->num_tracks; i++) {
    track_t *output_track = 0;
    output_track = malloc(sizeof(track_t));
    assert(output_track != NULL);
    char type[5] = {0};
    int check_return = fread(&type, 4, 1, file);
    assert(check_return == 1);
    assert(strcmp(type, TRACK_CHUNK) == 0);
    uint32_t length = 0;
    check_return = fread(&length, 4, 1, file);
    assert(check_return == 1);

    /* The fisrt part is very similar with parse_head, except that we are
     * readint tracks now and the header should be "MTrk". The following
     * code reverse the endianess of length, which represents the number
     * of bytes all the following events will take. Then we have a while
     * loop for the event linkedlist.
     */

    length = ((length>>24)&0xff) | ((length<<8)&0xff0000) |
    ((length>>8)&0xff00) | ((length<<24)&0xff000000);
    int position = ftell(file);
    event_node_t *event_root = 0;
    int cur_pos = position + 1;
    int flag_e = NOT_WRITTEN;
    event_node_t *head = event_root;
    while ((cur_pos - position) < length) {

      /* In this loop, we will keep reading events until
       * the bytes used by all events equals exactly the length we
       * expect from the track info we read previously. The following
       * code will read an event using parse_event, and will add the
       * event at the tail of the event linkedlist.
       */

      event_node_t *cur_event = 0;
      cur_event = malloc(sizeof(event_node_t));
      assert(cur_event != NULL);
      event_t *event_out = parse_event(file);
      cur_event->event = event_out;
      cur_event->next_event = NULL;
      if (flag_e == NOT_WRITTEN) {
        event_root = cur_event;
        head = event_root;
        flag_e = WRITTEN;
      }
      else {
        head->next_event = cur_event;
        head = head->next_event;
      }
      cur_pos = ftell(file);
    }
    output_track->length = (uint32_t)(cur_pos - position);
    output_track->event_list = event_root;
    if (flag == NOT_WRITTEN) {
      track_root->track = output_track;
      head_track->track = output_track;
      head_track->next_track = NULL;
      flag = WRITTEN;
    }
    else {
      track_node_t *cur_track_node = 0;
      cur_track_node = malloc(sizeof(track_node_t));
      assert(cur_track_node != NULL);
      cur_track_node->track = output_track;
      cur_track_node->next_track = NULL;
      head_track->next_track = cur_track_node;
      head_track = head_track->next_track;
    }
  }
  output->track_list = track_root;
} /* parse_track() */

/* parse_event() will take a file pointer, and will return a
 * event_t type pointer which points to the event_t variable
 * created in this function, WHICH is also filled with the
 * corresponding event data. Event can be either MIDI, META and
 * SYSTEM events. All the other events are considered invalid.
 */

event_t *parse_event(FILE *file) {
  uint8_t type_event_t = 0;
  event_t *output_eve = malloc(sizeof(event_t));
  assert(output_eve != NULL);
  uint32_t length_delta = 0;

  /* Delta time is a VLQ, so we will read it
   * it using parse_var_len(file)
   */

  length_delta = parse_var_len(file);
  output_eve->delta_time = length_delta;
  int check_return = fread(&type_event_t, 1, 1, file);
  assert(check_return == 1);

  if ((type_event_t == SYS_EVENT_1) || (type_event_t == SYS_EVENT_2)) {
    output_eve->sys_event = parse_sys_event(file);
  }

  else if(type_event_t == META_EVENT) {
    meta_event_t me_event =  parse_meta_event(file);
    output_eve->meta_event = (meta_event_t) me_event;
  }

  /* If it is not META or SYS event, it gotta be MIDI. If it failed
   * in parse_midi, then it is invalid
   */

  else {
    midi_event_t mi_event = parse_midi_event(file, type_event_t);
    output_eve->midi_event = (midi_event_t) mi_event;
    if ((type_event_t & 0b10000000) == 0) {
      type_event_t = g_midi_statue;
    }
  }
  output_eve->type = type_event_t;
  return output_eve;
} /* parse_event() */

/* parse_sys_event() will take a file pointer and assuming the
 * given file pointer is right next to a system event data. It
 * will read in the system event data and return a sys_event_t
 * type variable
 */

sys_event_t parse_sys_event(FILE *file) {
  sys_event_t output_sys = {0};
  uint32_t num_byte = parse_var_len(file);
  output_sys.data_len = num_byte;
  output_sys.data = malloc(num_byte);
  assert(output_sys.data != NULL);
  int check_return = fread(output_sys.data, num_byte, 1, file);
  assert(check_return == 1);
  return output_sys;
} /* parse_sys_event() */

/* parse_meta_event() will take a file pointer and assuming the
 * given file pointer is right next to a meta event data. It
 * will firstly find a corresponding meta event in the given
 * META_TABLE and assert the event exist. It will read in the
 * data with the given data length and return a meta_event_t
 * type variable
 */

meta_event_t parse_meta_event(FILE *file) {
  assert(file != NULL);
  build_event_tables();
  meta_event_t output_meta = {0};
  uint16_t type = 0;
  int check_return = fread(&type, 1, 1, file);
  assert(check_return == 1);
  output_meta = META_TABLE[type];
  assert(output_meta.name != NULL);
  uint32_t len_file = 0;
  len_file = parse_var_len(file);
  if (output_meta.data_len == 0) {
    output_meta.data_len = len_file;
  }
  else {
    assert(len_file == output_meta.data_len);
  }
  if (len_file == 0) {
    return output_meta;
  }
  else{
    output_meta.data = malloc(output_meta.data_len);
    assert(output_meta.data != NULL);
    check_return = fread(output_meta.data,
    output_meta.data_len, 1, file);
    assert(check_return == 1);
    return output_meta;
  }
} /* parse_meta_event() */

/* parse_midi_event() will take a file pointer which is assumed to
 * be pointing right next to a midi event. If the midi statue is
 * omitted, it will use the global midi statue and get a midi event
 * from MIDI_TABLE. If not, it will directly use stat to get a midi
 * event from MIDI_TABLE. Then it will read in data according to
 * the length in the midi_event and return it
 */

midi_event_t parse_midi_event(FILE *file, uint8_t stat) {
  assert(file != NULL);
  uint8_t type_event = stat;
  if ((stat & 0b10000000) == 0x00) {
    type_event = g_midi_statue;
  }
  else {
    g_midi_statue = type_event;
  }
  build_event_tables();
  midi_event_t midi_output = {0};
  midi_output = MIDI_TABLE[type_event];
  assert(midi_output.name != NULL);
  assert(midi_output.data_len != 0);
  midi_output.data = malloc(midi_output.data_len);
  assert(midi_output.data != NULL);
  int check_return = 0;
  uint8_t buffer = 0;

  /* If the first bit is 0, then it must be omitted, which means
   * we already read in 1 byte of the data. We fix this by reading
   * data_len - 1 bytes of data and connect them using pointer
   */

  if ((stat & 0b10000000) == 0) {
    midi_output.data[0] = stat;
    for (int i = 0; i < midi_output.data_len - 1; i++) {
      check_return = fread(&buffer, 1, 1, file);
      assert(check_return == 1);
      midi_output.data[i + 1] = buffer;
    }
  }
  else {
    int check_return = fread(midi_output.data,
    midi_output.data_len, 1, file);
    assert(check_return == 1);
  }
  return midi_output;
} /* parse_midi_event() */

/* parse_var_len() will take a file pointer, which is assumed to
 * have a VLQ right next to its pointer. It will read in the VLQ
 * and convert it to normal numbers as well as return it
 */

uint32_t parse_var_len(FILE *file) {
  uint8_t buff[1];
  uint32_t output=0;
  do {
    int check_return = fread(buff, 1, 1, file);
    assert(check_return == 1);

    /* Get rid of first bit, add stuff together
     * all in one line!
     */

    output = (output << 7) + (*buff & 0x7F);
  } while(*buff & 0x80);
  return output;
} /* parse_var_len() */

/* event_type() will take in an event_t type pointer
 * and will return the type of that event. Simple.
 */

uint8_t event_type(event_t *event_in) {
  assert(event_in != NULL);
  if ((event_in->type == SYS_EVENT_1) ||
  (event_in->type == SYS_EVENT_2)) {
    return SYS_EVENT_T;
  }
  else if (event_in->type == META_EVENT) {
    return META_EVENT_T;
  }
  else {
    return MIDI_EVENT_T;
  }
} /* event_type() */

/* free_song will take a midi_song pointer as parameter and
 * will free all the memory associated with the given song.
 */

void free_song(song_data_t *midi_song) {
  assert(midi_song != NULL);
  free(midi_song->path);
  midi_song->path = NULL;
  track_node_t *next = midi_song->track_list->next_track;
  while (next != NULL) {
    free_track_node(midi_song->track_list);
    midi_song->track_list = next;
    next = next->next_track;
  }
  free_track_node(midi_song->track_list);
  midi_song->track_list = NULL;
  free(midi_song);
  midi_song = NULL;
} /* free_song() */

/* free_track_node() will take in a track_node_t type pointer
 * and assuming it is the head of a linkedlist, it will free
 * the entire list and all associate events
 */

void free_track_node(track_node_t *track_root) {
  assert(track_root != NULL);
  event_node_t *next = track_root->track->event_list->next_event;
  while (next != NULL) {
    free_event_node(track_root->track->event_list);
    track_root->track->event_list = next;
    next = next->next_event;
  }
  free_event_node(track_root->track->event_list);
  track_root->track->event_list = NULL;
  free(track_root->track);
  track_root->track = NULL;
  track_root->next_track = NULL;
  free(track_root);
  track_root = NULL;
} /* free_track_node() */

/* free_event_node() will take a event_node_t type pointer
 * Assuming it is the head of the event chainlist, it will
 * free the entire list including all the events associated
 * with it.
 */

void free_event_node(event_node_t *event_root) {
  assert(event_root != NULL);
  if (event_type(event_root->event) == SYS_EVENT_T) {
    free(event_root->event->sys_event.data);
    event_root->event->sys_event.data = NULL;
  }
  else if (event_type(event_root->event) == META_EVENT_T) {
    free(event_root->event->meta_event.data);
    event_root->event->meta_event.data = NULL;
  }
  else {
    free(event_root->event->midi_event.data);
    event_root->event->midi_event.data = NULL;
  }
  free(event_root->event);
  event_root->event = NULL;
  event_root->next_event = NULL;
  free(event_root);
  event_root = NULL;
} /* free_event_node() */

/* end_swap_16 will take in a uint8_t type array
 * and will swap the position of the bytes inside
 * the array and return a uint16_t combined by the two
 * It will eventually works to swap the
 * endianness due to how system will swap it for us
 */

uint16_t end_swap_16(uint8_t original[2]) {
  uint16_t temp_big = original[0] << 8;
  uint16_t output = temp_big | original[1];
  return output;
} /* end_swap_16() */

/* end_swap_16 will take in a uint16_t type array
 * and will swap the position of the bytes inside
 * the array and return a uint32_t combined by these
 * two It will eventually works to swap the
 * endianness due to how system will swap it for us
 */

uint32_t end_swap_32(uint8_t original[4]) {
  uint32_t first = original[0] << 24;
  uint32_t second = original[1] << 16;
  uint16_t third = original[2] << 8;
  uint32_t output = first | second | third | original[3];
  return output;
} /* end_swap_32() */
