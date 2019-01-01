#include "block.h"
#include "gameinstance.h"
#include <string>
#include <sstream>
#include <QPushButton>
#include <QMessageBox>

using std::string;
using std::ostringstream;

Block::Block(QWidget *_parent,
             int _y,
             int _x,
             GameInstance *_host_game,
             int _type,
             int _orientation):
    QPushButton(_parent),
    host_game(_host_game),
    x(_x),
    y(_y),
    highlighted(false),
    orientation(_orientation),
    type(static_cast<BlockType>(_type))
{
    setText("");
    setAutoFillBackground(true);
    setFlat(true);
    setGeometry(QRect(NORMAL_X + BUTTON_WIDTH * _x, NORMAL_Y + BUTTON_HEIGHT * _y, BUTTON_WIDTH, BUTTON_HEIGHT));
    setStyleSheet("border: none");
    set_image(get_path());
    setVisible(true);

    // Custom initialise block flow directions
    this->setProperties(this->get_type(), this->get_orientation());
}

void Block::setProperties(BlockType type, int orientation) {
    this->type = type;

    this->orientation = orientation;

    switch (this->get_type()) {
    case BlockType::TJUNCTION:
        this->direction = UP | RIGHT | DOWN; break;
    case BlockType::TURN:
        this->direction = UP | RIGHT; break;
    case BlockType::STRAIGHT:
        this->direction = LEFT | RIGHT; break;
    case BlockType::CROSS:
        this->direction = LEFT | UP | RIGHT | DOWN; break;
    case BlockType::EMPTY:
        this->direction = 0;
    }

    this->direction <<= this->orientation;
    this->direction += this->direction / (1 << 4);
    this->direction %= 1 << 4;
}

string Block::get_path()
{
    ostringstream buf;
    if (highlighted) {
        buf << ":/resources/images/blocks_jpg/block" << type << "_" << orientation << "_f.jpg";
    } else {
        buf << ":/resources/images/blocks_jpg/block" << type << "_" << orientation << ".jpg";
    }
    return buf.str();
}

void Block::set_image(string path)
{
    setStyleSheet(QString::fromStdString("border-image: url(\"" + path + "\");"));
}

void Block::pressed()
{
    host_game -> block_pressed(y, x);
}

void Block::updateImage() {
    this->set_image(this->get_path());
}

void Block::rotate()
{
    ++this->orientation;
    this->orientation %= 4;
    this->updateImage();

    this->direction <<= 1;
    this->direction |= this->direction / (1 << 4);
    this->direction %= 1 << 4;
}

void Block::set_highlighted(bool value)
{
    this->highlighted = value;
}

bool Block::get_highlighted()
{
    return this->highlighted;
}

int Block::get_orientation()
{
    return this->orientation;
}

int Block::get_direction() {
    return this->direction;
}

BlockType Block::get_type()
{
    return this->type;
}
