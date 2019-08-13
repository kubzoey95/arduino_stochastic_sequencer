#define NO_TRACKS 4
#define NO_STEPS 16
#define MAX_PROB 100
#define EMPTY_NOTE -1
#define MODE_INPUT 1
#define NEXT_INPUT 2
#define PREVIOUS_INPUT 3
#define TRACK_CHANGE_INPUT 4
#define VALUE_INPUT 5
#define NO_OF_BTNS 4
#define NO_MODES 3
#define NOTE_CHANGE_MODE 0
#define PROB_CHANGE_MODE 1
#define TEMPO_CHANGE_MODE 2

class Step
{
  private:
    int note = -1;
    int prob = 100;
    int velo = 127;
  public:
    int getNotePlayed(){

      if (this->note == -1){
        return -1;
      }

      randomSeed(analogRead(0) + micros());
      long randNumber = random(101);
      Serial.println(randNumber);
      if(randNumber < this->prob){
        return this->note;
      }
      else{
        return -1;
      }
    }
};

class Track
{
  private:
    bool on = true;
    int channel = 0;
    int no_steps = NO_STEPS;
    Step* steps[NO_STEPS];
  public:
    void switchTrack(){
      this->on = !this->on;
    }
    void setChannel(int channel){
      this->channel = channel;
    }
};

class Sequencer
{
private:
  int sequence[NO_TRACKS][NO_STEPS] = {{EMPTY_NOTE}};
  int probs[NO_TRACKS][NO_STEPS] = {{50}};
  int velos[NO_TRACKS][NO_STEPS] = {{127}};
  bool track_on[NO_TRACKS] = {true};
  int tracks_channels[NO_TRACKS] = {0};
  int no_steps;
  int no_tracks;
  int curr_pos;
  int curr_edit_track;
  int curr_edit_step;
  bool plays;
  int mode;
  bool button_pushd;
  long button_wait;
  int button_waited;
  bool buttons_stats[NO_OF_BTNS] = {false};
  bool any_pushd = false;
  long last_btn_push_time;
  int bpm = 60;
  long beat_time_long;
public:
  Sequencer(){
    this->beat_time_long = micros();
    this->last_btn_push_time = micros();
    this->no_steps = NO_STEPS;
    this->no_tracks = NO_TRACKS;
    
    for(int i=0;i<NO_TRACKS;i++){
      for(int j=0;j<NO_STEPS;j++){
        this->sequence[i][j] = EMPTY_NOTE;
      }
    }
    for(int i=0;i<NO_TRACKS;i++){
      for(int j=0;j<NO_STEPS;j++){
        this->velos[i][j] = 127;
      }
    }
    this->curr_pos = 0;
    this->curr_edit_track = 0;
    this->curr_edit_step = 0;
    this->plays = false;
    this->mode = false;
    this->button_pushd = false;
    this->button_wait = 500000;
    this->button_waited = 0;
    }
  void setSequenceAt(int track, int st, int value){
    this->sequence[track][st] = value;
  }
  void setProbAt(int track, int st, int value){
    this->probs[track][st] = value % (MAX_PROB + 1);
  }
  void setTrackOnOffAt(int track, bool value){
    this->track_on[track] = value;
  }

  bool getNotePlayedAt(int track, int st){
    if (this->sequence[track][st] == -1){
      return false;
    }
    randomSeed(analogRead(0));
    long randNumber = random(101);
    Serial.println(randNumber);
    Serial.println(this->sequence[track][st]);
    Serial.println(track);
    Serial.println(st);
    if(track_on[track] && randNumber < this->probs[track][st]){
      return true;
    }
    else{
      return false;
    }
  }
  void noteOn(int channel, int pitch, int velocity) {
    Serial.write(144 + channel);
    Serial.write(pitch);
    Serial.write(velocity);
  }
  void noteOff(int channel, int pitch, int velocity){
    Serial.write(128 + channel);
    Serial.write(pitch);
    Serial.write(velocity);
  }

  void scheduleNotesOn(){
    for(int i=0;i<this->no_tracks;i++){
      if (this->getNotePlayedAt(i, this->curr_pos)){
        this->noteOn(this->tracks_channels[i], this->sequence[i][this->curr_pos], this->velos[i][this->curr_pos]);
      }
    }
  }

  void scheduleNotesOff(){
    for(int i=0;i<this->no_tracks;i++){
      this->noteOff(this->tracks_channels[i], this->sequence[i][this->curr_pos], this->velos[i][this->curr_pos]);
    }
  }

  void nextStep(){
    Serial.println("bang");
    this->curr_pos = (this->curr_pos + 1) % this->no_steps;
    this->scheduleNotesOn();
    this->beat_time_long = micros();
  }

  int resetStep(){
    this->scheduleNotesOff();
    this->curr_pos = 0;
    this->scheduleNotesOn();
    return this->curr_pos;
  }

  void switchPlays(){
    this->plays = !(this->plays);
    if (this->plays){
      this->scheduleNotesOn();
    }
    this->beat_time_long = micros();
    Serial.println(this->beat_time_long - micros());
  }
  
  void maybePlay(){
    if(this->plays){
      if(micros() - this->beat_time_long >= 60000000. / (float)this->bpm){
        this->scheduleNotesOff();
        this->nextStep();
      }
      }
  }

  void switchMode(){
    this->mode = (this->mode + 1) % NO_MODES;
  }

  int editTrackNext(){
    this->curr_edit_track = (this->curr_edit_track + 1) % this->no_tracks;
    return this->curr_edit_track;
  }

  void switchEditTrack(){
    this->track_on[this->curr_edit_track] = !this->track_on;
  }

  int editStepNext(){
    this->curr_edit_step = (this->curr_edit_step + 1) % this->no_steps;
    return this->curr_edit_step;
  }

  int editStepPrevious(){
    this->curr_edit_step = (this->curr_edit_step - 1) % this->no_steps;
    return this->curr_edit_step;
  }

  void updateBtnWaitedTime(){
    if(this->any_pushd){
      this->button_waited = min(micros() - this->last_btn_push_time, this->button_wait);
    }
    else{
      this->button_waited = 0;
    }
  }

  bool waitedTooLong(){
    this->updateBtnWaitedTime();
    return this->button_waited == this->button_wait;
  }

  void readButtonPush(int sens){
    bool any_pushd = false;
    for(int i=0;i<NO_OF_BTNS;i++){
      if(analogRead(i+1) > sens){
        if(!this->waitedTooLong()){
          this->buttons_stats[i] = true;
        }
        any_pushd = true;
      }
    }
    if(!this->any_pushd && any_pushd){
      this->last_btn_push_time = micros();
    }
    this->button_pushd = this->button_pushd && any_pushd;
    this->any_pushd = any_pushd;
  }

  bool btnComboReady(int btns[], int count){
    bool btns_ready = true;
    bool btns_together[NO_OF_BTNS] = {false};
    if(this->button_pushd){
      return false;
    }
    if(!this->waitedTooLong()){
      return false;
    }
    for(int i=0;i<count;i++){
      btns_together[btns[i]-1] = true;
    }
    for(int i=0;i<NO_OF_BTNS;i++){
      if(btns_together[i] != this->buttons_stats[i]){
        return false;
      }
    }
    this->button_pushd = true;
    return true;
  }
  
  void maybeNextTrack(){
    int next[1] = {TRACK_CHANGE_INPUT};
    if(this->btnComboReady(next, 1)){
      this->editTrackNext();
    }
  }
  void maybeSwitchPlays(){
    int switch_plays[2] = {NEXT_INPUT, PREVIOUS_INPUT};
    if(this->btnComboReady(switch_plays, 2)){
      this->switchPlays();
    }
  }
  void maybeNextBack(){
    int next[1] = {NEXT_INPUT};
    int prev[1] = {PREVIOUS_INPUT};
    if(this->btnComboReady(next, 1)){
      this->editStepNext();
      return;
    }
    else if(this->btnComboReady(prev, 1)){
      this->editStepPrevious();
      return;
    }
  }
  void maybeChangeMode(){
    int mode[] = {MODE_INPUT};
    if(this->btnComboReady(mode, 1)){
      Serial.println("mode changed");
      this->switchMode();
    }
  }

  void maybeProbEdit(){
    if(this->mode == PROB_CHANGE_MODE){
      this->probs[this->curr_edit_track][this->curr_edit_step] = ((float)analogRead(VALUE_INPUT) / 1024.) * 100.;
    }
  }

  void maybeNoteEdit(){
    if(this->mode == NOTE_CHANGE_MODE){
      this->sequence[this->curr_edit_track][this->curr_edit_step] = (int)(((float)analogRead(VALUE_INPUT) / 1024.) * 88.);
    }
  }

  void maybeTempoEdit(){
    if(this->mode == TEMPO_CHANGE_MODE){
      this->bpm == (int)(((float)analogRead(VALUE_INPUT) / 1024.) * 500.);
    }
  }
  
};

Sequencer *seq;


void setup() {
  Serial.begin(9600);
  Serial.println("begin");
  seq = new Sequencer();
  seq->switchPlays();
}

void loop() {
  seq->readButtonPush(800);
  seq->maybePlay();
  seq->maybeChangeMode();
  seq->maybeSwitchPlays();
  seq->maybeNoteEdit();
  seq->maybeProbEdit();
  seq->maybeTempoEdit();
  seq->maybeNextBack();
  seq->maybeNextTrack();
}
