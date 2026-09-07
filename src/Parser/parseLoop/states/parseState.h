#ifndef PARSE_STATE_H
#define PARSE_STATE_H

// this will be shift , reduce , accept , error
// stores what the last state was that we fetched 
class ParseState {
public:
  virtual bool isShift() { return false; }
  virtual bool isReduce() { return false; };
  virtual bool isAccept() { return false; };
  virtual bool isError() { return false; };
};

class Shift : public ParseState {
  virtual bool isShift() override { return true; };
};
class Reduce : public ParseState {
  virtual bool isReduce() override { return true; };
};

class Accept : public ParseState {
  virtual bool isAccept() override { return true; };
};

class Error : public ParseState {
  virtual bool isError() override { return true; };
};
#endif
