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

class Sequencer
{
private:
  int sequence[NO_TRACKS][NO_STEPS] = {{0}};
  int probs[NO_TRACKS][NO_STEPS] = {{0}};
  bool track_on[NO_TRACKS] = {true};
  int no_steps;
  int no_tracks;
  int curr_pos;
  int curr_edit_track;
  int curr_edit_step;
  bool plays;
  bool edits_prob;
  bool button_pushd;
  long button_wait;
  int button_waited;
  bool buttons_stats[NO_OF_BTNS] = {false};
  bool any_pushd = false;
  long last_btn_push_time;
  int bpm = 120;
  long beat_time_long;
public:
  Sequencer(){
    this->beat_time_long = micros();
    this->last_btn_push_time = micros();
    this->no_steps = NO_STEPS;
    this->no_tracks = NO_TRACKS;
    for(int i=0;i<NO_TRACKS;i++){
      for(int j=0;i<NO_STEPS;j++){
        this->sequence[i][j] = EMPTY_NOTE;
      }
    }
    this->curr_pos = 0;
    this->curr_edit_track = 0;
    this->curr_edit_step = 0;
    this->plays = false;
    this->edits_prob = false;
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

  int getNoteAt(int track, int st){
    randomSeed(analogRead(0));
    long randNumber = random(101);
    if(track_on[track] && randNumber >= this->probs[track][st]){
      return this->sequence[track][st];
    }
    else{
      return -1;
    }
  }

  void nextStep(){
    this->curr_pos = (this->curr_pos + 1) % this->no_steps;
    this->beat_time_long = micros();
  }

  int resetStep(){
    this->curr_pos = 0;
    return this->curr_pos;
  }

  void switchPlays(){
    this->plays = !(this->plays);
    this->beat_time_long = micros();
  }

  void maybePlay(){
    if(this->plays){
      if(this->beat_time_long - micros() >= (60. / (float)this->bpm) * 1000000.){
        // send midi
        this->nextStep();
      }
      }
  }

  void switchEditsProb(){
    this->edits_prob = !(this->edits_prob);
  }

  int editTrackNext(){
    this->curr_edit_track = (this->curr_edit_track + 1) % this->no_tracks;
    return this->curr_edit_track;
  }

  void switchEditTrack(){
    this->track_on[this->curr_edit_track] = !this->track_on;
  }

  int editStepNext(){
    this->curr_edit_step = (this->curr_edit_step + 1) % this->no_tracks;
    return this->curr_edit_step;
  }

  int editStepPrevious(){
    this->curr_edit_step = (this->curr_edit_step - 1) % this->no_tracks;
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
  void maybeNextBack(){
    int next[] = {NEXT_INPUT};
    int prev[] = {PREVIOUS_INPUT};
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
      this->switchEditsProb();
    }
  }

  void maybeProbEdit(){
    if(this->edits_prob){
      this->probs[this->curr_edit_track][this->curr_edit_step] = ((float)analogRead(VALUE_INPUT) / 1024.) * 100.;
    }
  }

  void maybeNoteEdit(){
    if(!this->edits_prob){
      this->sequence[this->curr_edit_track][this->curr_edit_step] = (int)(((float)analogRead(VALUE_INPUT) / 1024.) * 88.);
    }
  }
  
};

Sequencer *seq;


void setup() {
  seq = new Sequencer();
}

void loop() {
  seq->readButtonPush(800);
  seq->maybePlay();
  seq->maybeChangeMode();
  seq->maybeNoteEdit();
  seq->maybeProbEdit();
  seq->maybeNextBack();
  seq->maybeNextTrack();
}
