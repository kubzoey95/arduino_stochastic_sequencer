#define NO_TRACKS 4
#define NO_STEPS 16
#define MAX_PROB 100
#define EMPTY_NOTE -1
#define MODE_INPUT 0
#define NEXT_INPUT 1
#define PREVIOUS_INPUT 2
#define TRACK_CHANGE_INPUT 3
#define NOTE_INPUT 5
#define VELO_INPUT 4
#define TEMPO_INPUT 2
#define PROB_INPUT 3
#define NO_OF_BTNS 4
#define NO_MODES 3
#define NOTE_CHANGE_MODE 0
#define PROB_CHANGE_MODE 1
#define TEMPO_CHANGE_MODE 2

class Step
{
  private:
    int note = -1;
    int prob = MAX_PROB;
    int velo = 127;
  public:
    void setNote(int note){
      this->note = min(note, 127);
    }

    void setProb(int prob){
      this->prob = min(prob, MAX_PROB);
    }

    void setVelo(int velo){
      this->note = min(127, velo);
    }

    bool getPlayed(){
      if (this->note == EMPTY_NOTE){
        return false;
      }

      randomSeed(analogRead(0) + micros());
      long randNumber = random(101);
      if(randNumber < this->prob){
        return true;
      }
      else{
        return false;
      }
    }

    int * getNote(){
      int result[] = {this->note, this->velo};
      return result;
    }

    void noteOn(int channel) {
    if (this->note == EMPTY_NOTE || !this->getPlayed()){
      return;
    }
//    Serial.println(channel);
//    Serial.println(this->note);
//    Serial.println(this->velo);
//    Serial.println(this->prob);
    
    Serial.write(144 + channel);
    Serial.write(this->note);
    Serial.write(this->velo);
  }
  void noteOff(int channel){
    if (this->note == EMPTY_NOTE){
      return;
    }
    Serial.write(144 + channel);
    Serial.write(this->note);
    Serial.write(0);
  }
};

class Track
{
  private:
    bool on = true;
    int channel = 0;
    int no_steps = NO_STEPS;
    Step* steps;
  public:
    Track(){
      this->steps = new Step[this->no_steps];
    }
    void switchTrack(){
      this->on = !this->on;
    }
    void setChannel(int channel){
      this->channel = channel;
    }

    int getChannel(){
      return this->channel;
    }
    void setStepNote(int step, int note){
      this->steps[step].setNote(note);
    }
    void setStepProb(int step, int prob){
      this->steps[step].setProb(prob);
    }
    void setStepVelo(int step, int velo){
      this->steps[step].setProb(velo);
    }
    int * getNoteAtStep(int step){
      return this->steps[step].getNote();
    }
    bool getPlayedAtStep(int step){
      return this->steps[step].getPlayed();
    }

    void noteOnAtStep(int step){
      if (this->on){
      this->steps[step].noteOn(this->channel);
      }
    }

    void noteOffAtStep(int step){
      if (this->on){
      this->steps[step].noteOff(this->channel);
      }
    }
};

class Button {
  private:
    int pin;
    bool pushed;
  public:
    Button(){
      this->pin = -1;
      this->pushed = false;
    }
    Button(int pin){
      pinMode(pin, OUTPUT);
      this->pin = pin;
      this->pushed = digitalRead(this->pin) == HIGH;
    }
    bool updateStatus(){
      bool pushed = digitalRead(this->pin) == HIGH;
      this->pushed = pushed;
      return pushed;
    }
};

class PotInput{
  private:
    int pin;
    float last_value;
    float scale;
  public:
  PotInput(){
    this->pin = -1;
    this->last_value = 0;
    this->scale = 127;
  }

  PotInput(int pin, float scale){
    this->pin = pin;
    this->last_value = min(analogRead(this->pin), 1024.);
    this->scale = scale;
  }

  float readValue(){
    return this->last_value;
  }

  bool newValue(){
    float reading = analogRead(this->pin);
    reading = 0.9 * this->last_value + 0.1 * reading;
    float dif = this->last_value - reading;
    if(abs(dif) / 1024.00000 > this->scale){
      this->last_value = reading;
      return true;
    }
    return false;
  }
};

class Sequencer
{
private:
  Track* tracks;
  Button** buttons;
  PotInput** pots;
  int no_steps;
  int no_tracks;
  int curr_pos;
  int curr_edit_track;
  int curr_edit_step;
  bool plays;
  int mode;
  bool action_taken;
  long button_wait;
  long button_waited;
  bool buttons_stats[NO_OF_BTNS] = {false};
  bool any_pushd = false;
  long last_btn_push_time;
  int bpm = 60;
  long beat_time_long;
public:
  Sequencer(){
    this->tracks = new Track[NO_TRACKS];
    this->buttons = new Button*[NO_OF_BTNS];
    this->pots = new PotInput*[4];
    this->buttons[0] = new Button(4);
    this->buttons[1] = new Button(7);
    this->buttons[2] = new Button(8);
    this->buttons[3] = new Button(12);
    this->pots[0] = new PotInput(NOTE_INPUT, 1. / 127.);
    this->pots[1] = new PotInput(VELO_INPUT, 1. / 127.);
    this->pots[2] = new PotInput(PROB_INPUT, 1. / 100.);
    this->pots[3] = new PotInput(TEMPO_INPUT, 1. / 100.);
    this->beat_time_long = micros();
    this->last_btn_push_time = micros();
    this->no_steps = NO_STEPS;
    this->no_tracks = NO_TRACKS;

    this->curr_pos = 0;
    this->curr_edit_track = 0;
    this->curr_edit_step = 0;
    this->plays = false;
    this->mode = false;
    this->action_taken = false;
    this->button_wait = 125000;
    this->button_waited = 0;
    }
  void setSequenceAt(int track, int st, int value){
    this->tracks[track].setStepNote(st, value);
  }
  void setProbAt(int track, int st, int value){
    this->tracks[track].setStepNote(st, min(value, MAX_PROB + 1));
  }

  void scheduleNotesOn(){
    for(int i=0;i<this->no_tracks;i++){
      this->tracks[i].noteOnAtStep(this->curr_pos);
    }
  }

  void scheduleNotesOff(){
    for(int i=0;i<this->no_tracks;i++){
      Serial.write(176 + this->tracks[i].getChannel());
      Serial.write(123);
      Serial.write(0);
//      this->tracks[i].noteOffAtStep(this->curr_pos);
    }
  }

  void nextStep(){
    this->scheduleNotesOff();
    this->curr_pos = ((this->curr_pos + 1) % this->no_steps + this->no_steps) % this->no_steps;
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
      this->beat_time_long = micros();
    }
    else{
      this->scheduleNotesOff();
    }
  }
  
  void maybePlay(){
    if(this->plays){
      if(micros() - this->beat_time_long >= 60000000. / (float)this->bpm){
        this->nextStep();
      }
      }
  }

  void switchMode(){
    this->mode = (this->mode + 1) % NO_MODES;
  }

  int editTrackNext(){
    this->curr_edit_track = ((this->curr_edit_track + 1) % this->no_tracks + this->no_tracks) % this->no_tracks;
    return this->curr_edit_track;
  }

  void switchEditTrack(){
    this->tracks[this->curr_edit_track].switchTrack();
  }

  int editStepNext(){
    this->curr_edit_step = ((this->curr_edit_step + 1) % this->no_steps + this->no_steps) % this->no_steps;
    return this->curr_edit_step;
  }

  int editStepPrevious(){
    this->curr_edit_step = ((this->curr_edit_step - 1) % this->no_steps + this->no_steps) % this->no_steps;
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

  bool updateAnyPushed(){
    for (int i = 0; i < NO_OF_BTNS; i++)
    {
      if(this->buttons[i]->updateStatus()){
        if(this->any_pushd){
          return true;
        }
        this->any_pushd = true;
        this->last_btn_push_time = micros();
        return true;
      }
    }
    this->any_pushd = false;
    this->action_taken = false;
    return false;
  }

  bool btnComboReady(int btns[], int count){
    bool btns_together[NO_OF_BTNS] = {false};
    if(this->action_taken){
      return false;
    }
    if(!this->waitedTooLong()){
      return false;
    }
    for(int i=0;i<count;i++){
      btns_together[btns[i]] = true;
    }
    for(int i=0;i<NO_OF_BTNS;i++){
      if(btns_together[i] != this->buttons[i]->updateStatus()){
        return false;
      }
    }
    this->action_taken = true;
    return true;
  }
  
  void maybeNextTrack(){
    int next[2] = {TRACK_CHANGE_INPUT, MODE_INPUT};
    if(this->btnComboReady(next, 2)){
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
  void maybeEraseAllFromTrack(){
    int erease[] = {MODE_INPUT, NEXT_INPUT};
    if(this->btnComboReady(erease, 2)){
      for (int i = 0; i < this->no_steps; i++)
      {
        this->tracks[this->curr_edit_track].setStepProb(i, 100);
       this->tracks[this->curr_edit_track].setStepNote(i, -1);
       this->tracks[this->curr_edit_track].setStepVelo(i, 0);
      }
    }
  }

  void maybeRandomTrack(){
    int rand[] = {PREVIOUS_INPUT, TRACK_CHANGE_INPUT};
    if(this->btnComboReady(rand, 2)){
      for (int i = 0; i < this->no_steps; i++)
      {
       randomSeed(analogRead(0) + micros());
       long prob = random(101);
       randomSeed(analogRead(0) + micros());
       long note = random(128);
       randomSeed(analogRead(0) + micros());
       long velo = 127;
       this->tracks[this->curr_edit_track].setStepProb(i, prob);
       this->tracks[this->curr_edit_track].setStepNote(i, note);
       this->tracks[this->curr_edit_track].setStepVelo(i, velo);
      }
    }
  }

  void maybeProbEdit(){
      int pot_no = 2;
        if(this->pots[pot_no]->newValue()){
          this->tracks[this->curr_edit_track].setStepProb(this->curr_edit_step, (int)(((float)this->pots[pot_no]->readValue() / 1024.) * 100.));
        }
  }

  void maybeNoteEdit(){
      int pot_no = 0;
      if(this->pots[pot_no]->newValue()){
        this->tracks[this->curr_edit_track].setStepNote(this->curr_edit_step, (int)(((float)this->pots[pot_no]->readValue() / 1024.) * 127.));
    }
  }

  void maybeVeloEdit(){
      int pot_no = 1;
      if(this->pots[pot_no]->newValue()){
        this->tracks[this->curr_edit_track].setStepVelo(this->curr_edit_step, (int)(((float)this->pots[pot_no]->readValue() / 1024.) * 127.));
    }
  }

  void maybeTempoEdit(){
      int pot_no = 3;
      if(this->pots[pot_no]->newValue()){
        this->bpm = this->pots[pot_no]->readValue() / 1024. * 500.;
      }
  }
};

Sequencer *seq;


void setup() {
  Serial.begin(31250);
  seq = new Sequencer();
  seq->switchPlays();
}

void loop() {
  seq->updateAnyPushed();
  seq->maybePlay();
  seq->maybeSwitchPlays();
  seq->maybeNoteEdit();
  seq->maybeProbEdit();
  seq->maybeTempoEdit();
  seq->maybeVeloEdit();
  seq->maybeNextBack();
  seq->maybeNextTrack();
  seq->maybeEraseAllFromTrack();
  seq->maybeRandomTrack();
}
