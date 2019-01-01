#ifndef GAMEINSTANCE_H
#define GAMEINSTANCE_H

#include <QString>
#include <QObject>

#include "block.h"

class GameWindow;

struct BFSNode {
    int from, y, x;
};

enum BFSStatus {
    CONNECTED, LEAKAGE, STUCK
};

struct BFSResult {
    BFSStatus status;
    int cycles;
};

struct BlockData {
    BlockType type;
    int orientation;
};

class GameInstance : public QObject
{
    Q_OBJECT

 public:
    GameInstance(int _level, int _min_step);
    ~GameInstance();
    void block_pressed(int y, int x);
    int get_result();

 private:

    static const QString map_path;
    static const int MAP_SIZE = 8;
    Block *blocks[MAP_SIZE][MAP_SIZE];
    GameWindow *game_gui;
    int used_step;
    int min_step;
    int level;
    int result;
    void init_block(int _type, int _orientation, int _y, int _x);
    void load_map(int dest_level);

    // BFS
    bool isChecking = false;
    static const int animateTime = 100;
    bool** initTravelled();
    void deleteTravelled(bool** &travelled);
    inline int opposite(int direction);
    inline int deltaY(int direction);
    inline int deltaX(int direction);
    void updateBlockImage(int y, int x, bool highlighted);
    BFSResult bfsBlocks(bool animate = false);

    // Feature added
    static const bool animationChangeEnabled = true;
    void loadFeatureMap();
    void randomAddPipe();
    BlockData** initBlockData();
    void deleteBlockData(BlockData** &blockData);
    void replace(BlockData** blockData);
    void combine(BlockData &destination, BlockData &part);
    void rotateClockwise(BlockData** blockData);
    void swipeLeft(BlockData** blockData);
    void swipeRight(BlockData** blockData);
    void swipeUp(BlockData** blockData) ;
    void swipeDown(BlockData** blockData);

 signals:
    void game_over();

 private slots:
    void on_done_button_clicked();
    void quit();
    void keyPressed(QKeyEvent *keyEvent);
};

#endif // GAMEINSTANCE_H
