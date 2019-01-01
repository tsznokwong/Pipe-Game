#include <QFile>
#include <QString>
#include <QObject>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <queue>

#include "gameinstance.h"
#include "gamewindow.h"
#include "loginwindow.h"

using namespace std;

const QString GameInstance::map_path = ":/resources/maps/maps.txt";

GameInstance::GameInstance(int _level, int _min_step):
    game_gui(new GameWindow(nullptr)),
    used_step(0),
    min_step(_min_step),
    level(_level),
    result(-1)
{
    game_gui -> show();
    game_gui -> set_lcd(GameWindow::USED_STEP_LCD, 0);
    game_gui -> set_lcd(GameWindow::MIN_STEP_LCD, _min_step == -1 ? 999 : _min_step);
    game_gui -> set_lcd(GameWindow::LEVEL_LCD, _level);
    load_map(_level);
    connect(game_gui -> get_done_button(), SIGNAL(clicked()), this, SLOT(on_done_button_clicked()));
    connect(game_gui, SIGNAL(closed()), this, SLOT(quit()));
    if (level == featureLevel) {
        connect(game_gui, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyPressed(QKeyEvent*)));
    }
}

void GameInstance::init_block(int _type, int _orientation, int _y, int _x)
{
    Block* ret = new Block(game_gui, _y, _x, this, _type, _orientation);
    connect(ret, SIGNAL(clicked()), ret, SLOT(pressed()));
    blocks[_y][_x] = ret;
}

void GameInstance::quit()
{
    emit game_over();
}

void GameInstance::load_map(int dest_level)
{
    if (dest_level == featureLevel) {
        this->loadFeatureMap();
        return;
    }

    QFile mapFile{this->map_path};
    mapFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QString data = mapFile.readAll();
    // Split by block
    QStringList mapData = data.split(QRegExp("(\\[|\\]\\n\\[|\\])"));
    QString destMapData = mapData[dest_level];

    QRegExp rex{"\\((\\d), (\\d)\\)"};
    int pos = 0;
    int block = 0;
    while ((pos = rex.indexIn(destMapData, pos)) != -1) {
        int type = rex.cap(1).toInt();
        int orientation = rex.cap(2).toInt();
        this->init_block(type, orientation, block / 8, block % 8);
        ++block;
        pos += rex.matchedLength();
    }

}


GameInstance::~GameInstance()
{
    delete this->game_gui;
    for (int x = 0; x < this->MAP_SIZE; ++x) {
        for (int y = 0; y < this->MAP_SIZE; ++y) {
            delete this->blocks[y][x];
        }
    }
}

void GameInstance::block_pressed(int y, int x)
{
    if (this->isChecking) return;
    if (this->blocks[y][x]->get_type() == BlockType::EMPTY) return;
    this->blocks[y][x]->rotate();
    ++used_step;
    this->game_gui->set_lcd(GameWindow::USED_STEP_LCD, this->used_step);
}

int GameInstance::get_result()
{
    return this->result;
}

// BFS
bool** GameInstance::initTravelled() {
    bool **travelled = new bool* [this->MAP_SIZE];
    for (int i = 0; i < this->MAP_SIZE; ++i) {
        travelled[i] = new bool [this->MAP_SIZE];
        for (int j = 0; j < this->MAP_SIZE; ++j) {
            travelled[i][j] = false;
        }
    }
    return travelled;
}

void GameInstance::deleteTravelled(bool** &travelled) {
    for (int i = 0; i < this->MAP_SIZE; ++i) {
        delete [] travelled[i];
    }
    delete [] travelled;
    travelled = nullptr;
}

inline int GameInstance::opposite(int direction) {
    direction <<= 2;
    direction |= direction / (1 << 4);
    direction %= 1 << 4;
    return direction;
}

inline int GameInstance::deltaY(int direction) {
    if (direction & (LEFT | RIGHT)) return 0;
    if (direction & UP) return -1;
    if (direction & DOWN) return 1;
    return 0;
}

inline int GameInstance::deltaX(int direction) {
    if (direction & (UP | DOWN)) return 0;
    if (direction & LEFT) return -1;
    if (direction & RIGHT) return 1;
    return 0;
}


BFSResult GameInstance::bfsBlocks(bool animate) {
    bool **travelled = this->initTravelled();
    BFSStatus status = BFSStatus::STUCK;
    queue<BFSNode> frontier;
    // initial frontier
    frontier.push({LEFT, 0, 0});

    int cycle = 1;
    while (!frontier.empty()) {
        BFSNode node = frontier.front();
        frontier.pop();

        // outlet position
        if (node.x == this->MAP_SIZE && node.y == this->MAP_SIZE - 1) {
            status = BFSStatus::CONNECTED;
            continue;
        }

        // Check range
        if (node.x < 0 || node.y < 0 || node.x >= this->MAP_SIZE || node.y >= this->MAP_SIZE) {
            status = BFSStatus::LEAKAGE;
            break;
        }

        Block *block = this->blocks[node.y][node.x];
        int blockDirection = block->get_direction();

        // Cannot flow from
        if ((node.from & blockDirection) == 0) {
            status = BFSStatus::LEAKAGE;
            break;
        }
        blockDirection -= node.from;

        // Check travelled
        if (travelled[node.y][node.x]) {
            continue;
        }

        travelled[node.y][node.x] = true;

        if (animate) {
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, [=]() {
                this->updateBlockImage(node.y, node.x, true);
                delete timer;
            });
            timer->start(this->animateTime * cycle);
        }

        for (int direction = LEFT; direction <= DOWN; direction <<= 1) {
            if (direction & blockDirection) {
                frontier.push({this->opposite(direction), node.y + this->deltaY(direction), node.x + this->deltaX(direction)});
            }
        }

        ++cycle;
    }

    this->deleteTravelled(travelled);
    return {status, cycle};
}

void GameInstance::updateBlockImage(int y, int x, bool highlighted) {
    this->blocks[y][x]->set_highlighted(highlighted);
    this->blocks[y][x]->updateImage();
}

void GameInstance::on_done_button_clicked()
{
    if (this->isChecking) return;
    this->isChecking = true;
    BFSResult result = this->bfsBlocks(this->animationChangeEnabled);

    QTimer *timer = new QTimer(this);
    switch (result.status) {
    case BFSStatus::LEAKAGE:
        connect(timer, &QTimer::timeout, [=]() {
            QMessageBox::information(nullptr, "", "There's leakage in the maze.\nGame Over!");
            this->isChecking = false;
            this->game_gui->close();
            delete timer;
        });
        break;
    case BFSStatus::STUCK:
        connect(timer, &QTimer::timeout, [=]() {
            QMessageBox::information(nullptr, "", "It seems the water can not flow into the outlet.\nGame Over!");
            this->isChecking = false;
            this->game_gui->close();
            delete timer;
        });
        break;
    case BFSStatus::CONNECTED:
        if (!this->animationChangeEnabled) {
            this->bfsBlocks(true);
        }
        this->result = this->used_step;
        connect(timer, &QTimer::timeout, [=]() {
            this->game_gui->set_outlet(true);
            QMessageBox::information(nullptr, "", "Congratulations!");
            this->isChecking = false;
            this->game_gui->close();
            delete timer;
        });
    }
    timer->start((this->animationChangeEnabled || result.status == BFSStatus::CONNECTED ? this->animateTime : 0) * result.cycles);

}


// Feature
void GameInstance::loadFeatureMap() {
    for (int y = 0; y < this->MAP_SIZE; ++y) {
        for (int x = 0; x < this->MAP_SIZE; ++x) {
            this->init_block(BlockType::EMPTY, 0, y, x);
        }
    }
    for (int i = 0; i < this->MAP_SIZE; ++i) {
        this->randomAddPipe();
    }
}

void GameInstance::randomAddPipe() {
    Block **emptyBlocks = new Block*[this->MAP_SIZE * this->MAP_SIZE];
    int size = 0;
    for (int y = 0; y < this->MAP_SIZE; ++y) {
        for (int x = 0; x < this->MAP_SIZE; ++x) {
            if (this->blocks[y][x]->get_type() == BlockType::EMPTY) {
                emptyBlocks[size++] = this->blocks[y][x];
            }
        }
    }
    if (size == 0) return;
    int index = qrand() % size;
    BlockType type = static_cast<BlockType>(qrand() % 2 + 2);
    int orientation = qrand() % 4;

    emptyBlocks[index]->setProperties(type, orientation);
    emptyBlocks[index]->updateImage();

}

BlockData** GameInstance::initBlockData() {
    BlockData **blockData = new BlockData* [this->MAP_SIZE];
    for (int y = 0; y < this->MAP_SIZE; ++y) {
        blockData[y] = new BlockData [this->MAP_SIZE];
        for (int x = 0; x < this->MAP_SIZE; ++x) {
            Block* block = this->blocks[y][x];
            blockData[y][x] = {block->get_type(), block->get_orientation()};
        }
    }
    return blockData;
}

void GameInstance::deleteBlockData(BlockData** &blockData) {
    for (int y = 0; y < this->MAP_SIZE; ++y) {
        delete [] blockData[y];
    }
    delete [] blockData;
    blockData = nullptr;
}

void GameInstance::combine(BlockData &destination, BlockData &part) {
    if (destination.type != part.type) return;
    switch (destination.type) {
    case BlockType::EMPTY:
    case BlockType::TURN:
        return;
    case BlockType::CROSS:
        destination.type = BlockType::TJUNCTION;
        break;
    case BlockType::TJUNCTION:
        destination.type = BlockType::STRAIGHT;
        break;
    case BlockType::STRAIGHT:
        destination.type = BlockType::TURN;
        break;
    }
    part.type = BlockType::EMPTY;
}

void GameInstance::rotateClockwise(BlockData** blockData) {
    const int size = this->MAP_SIZE;
    BlockData **result = this->initBlockData();

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            result[j][size - i - 1] = blockData[i][j];
        }
    }
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            blockData[i][j] = result[i][j];
        }
    }
    this->deleteBlockData(result);
}

void GameInstance::swipeLeft(BlockData** blockData) {
    const int size = this->MAP_SIZE;
    for (int y = 0; y < size; ++y) {
        BlockType leftType = BlockType::EMPTY;
        int leftIndex = -1;
        for (int x = 0; x < size; ++x) {
            if (blockData[y][x].type == BlockType::EMPTY) {
                continue;
            }
            if (leftType == BlockType::EMPTY || blockData[y][x].type != leftType) {
                leftType = blockData[y][x].type;
                leftIndex = x;
            } else {
                combine(blockData[y][leftIndex], blockData[y][x]);
                leftType = BlockType::EMPTY;
                leftIndex = -1;
            }
        }

        int count = 0;
        for (int x = 0; x < size; ++x) {
            if (blockData[y][x].type != BlockType::EMPTY) {
                blockData[y][count++] = blockData[y][x];
            }
        }
        for (; count < size; ++count) {
            blockData[y][count] = {BlockType::EMPTY, 0};
        }
    }
}


void GameInstance::swipeRight(BlockData** blockData) {
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
    this->swipeLeft(blockData);
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
}


void GameInstance::swipeUp(BlockData** blockData) {
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
    this->swipeLeft(blockData);
    this->rotateClockwise(blockData);
}


void GameInstance::swipeDown(BlockData** blockData) {
    this->rotateClockwise(blockData);
    this->swipeLeft(blockData);
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
    this->rotateClockwise(blockData);
}

void GameInstance::replace(BlockData** blockData) {
    for (int y = 0; y < this->MAP_SIZE; ++y) {
        for (int x = 0; x < this->MAP_SIZE; ++x) {
            this->blocks[y][x]->setProperties(blockData[y][x].type, blockData[y][x].orientation);
            this->blocks[y][x]->updateImage();
        }
    }
    this->randomAddPipe();

}

void GameInstance::keyPressed(QKeyEvent *keyEvent) {
    BlockData **blockData = this->initBlockData();
    switch (keyEvent->key()) {
    case Qt::Key::Key_Left:
        this->swipeLeft(blockData);
        this->replace(blockData); break;
    case Qt::Key::Key_Right:
        this->swipeRight(blockData);
        this->replace(blockData); break;
    case Qt::Key::Key_Up:
        this->swipeUp(blockData);
        this->replace(blockData); break;
    case Qt::Key::Key_Down:
        this->swipeDown(blockData);
        this->replace(blockData); break;
    }

    this->deleteBlockData(blockData);
}






