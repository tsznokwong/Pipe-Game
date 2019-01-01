#include "loginwindow.h"
#include "gameinstance.h"
#include "recordmanager.h"
#include "ui_loginwindow.h"
#include <QMessageBox>
#include <QTime>

#include <sstream>

using std::ostringstream;

LoginWindow::LoginWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::LoginWindow),
    rm(new RecordManager()),
    current_level(1),
    started(false)
{
    ui -> setupUi(this);
    qsrand(static_cast<uint>(QTime::currentTime().msec()));
}

LoginWindow::~LoginWindow()
{
    delete rm;
    delete ui;
}

void LoginWindow::set_statusbar_text(string str)
{
    ui -> statusBar -> showMessage(QString::fromStdString(str));
}

void LoginWindow::refresh_background()
{
    ostringstream buf;
    buf << "#centralWidget { border-image: url(\":/resources/images/login_pic/level_" << current_level << ".png\"); }";
    ui -> centralWidget -> setStyleSheet(QString::fromStdString(buf.str()));
}

void LoginWindow::start_game()
{
    game = new GameInstance(current_level, rm -> get_record(current_level));
    connect(game, SIGNAL(game_over()), this, SLOT(game_closed()));
}

void LoginWindow::game_closed()
{
    // update record if needed
    int minimumStep = this->game->get_result();
    int level = (this->startedFeature ? featureLevel : this->current_level);
    int previous = rm->get_record(level);
    if (previous == -1 || (minimumStep != -1 && minimumStep < previous))
        rm->update_record(level, minimumStep);

    this->started = false;
    this->startedFeature = false;
}

void LoginWindow::on_prev_button_clicked()
{
    if (this->started) return;

    if (this->current_level == 1) {
        this->set_statusbar_text("You are already at the minimum level.");
        return;
    }
    --this->current_level;
    this->refresh_background();
    this->set_statusbar_text("");
}

void LoginWindow::on_next_button_clicked()
{
    if (this->started) return;

    if (this->current_level == this->rm->get_num_of_levels()) {
        this->set_statusbar_text("You are already at the maximum level.");
        return;
    }
    if (rm->get_record(this->current_level) == -1) {
        this->set_statusbar_text("You can not move to next level before passing this level.");
        return;
    }

    ++this->current_level;
    this->refresh_background();
    this->set_statusbar_text("");
}

void LoginWindow::on_start_button_clicked()
{
    if (this->started) return;

    this->start_game();
    this->started = true;
}

void LoginWindow::on_feature_button_clicked() {
    if (this->started) return;
    this->start_feature_game();

    this->started = true;
    this->startedFeature = true;
    QMessageBox::information(nullptr, "New Game Mode", "Using arrow keys to move pipes, like 2048!!");
}

void LoginWindow::start_feature_game()
{
    game = new GameInstance(featureLevel, rm->get_record(featureLevel));
    connect(game, SIGNAL(game_over()), this, SLOT(game_closed()));
}
