#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <QPushButton>

using std::string;

// Bit mask info
static const int LEFT   = 1 << 0;
static const int UP     = 1 << 1;
static const int RIGHT  = 1 << 2;
static const int DOWN   = 1 << 3;

// Type info
enum BlockType {
    TJUNCTION, TURN, STRAIGHT, CROSS, EMPTY
};

class GameInstance;

class Block : public QPushButton
{
    Q_OBJECT

 public:
    Block(QWidget* _parent = nullptr,
          int _y = 0,
          int _x = 0,
          GameInstance* _host_game = nullptr,
          int _type = 0,
          int _orientation = 0);

    void rotate();
    void set_highlighted(bool value);
    void set_image(string path);
    void updateImage();

    string get_path();
    bool get_highlighted();
    int get_orientation();
    BlockType get_type();
    void setProperties(BlockType type, int orientation);
    int get_direction();

 private:
    static const int NORMAL_X = 117;
    static const int NORMAL_Y = 146;
    static const int BUTTON_HEIGHT = 58;
    static const int BUTTON_WIDTH = 58;

    GameInstance *host_game;
    int x;
    int y;

    bool highlighted;
    int orientation;
    BlockType type;
    int direction;

 private slots:
    void pressed();
};

#endif // BLOCK_H
