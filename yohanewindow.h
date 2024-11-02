#ifndef YOHANEWINDOW_H
#define YOHANEWINDOW_H

#define PLAYER_POINT_BASE_ADDR1     0x115D694
#define PLAYER_POINT_BASE_ADDR2     0x0115B498
#define PLAYER_COINS_BASE_ADDR      0x0166B418

#define HP_VALUE_OFFSETS    { 0x28, 0x38, 0x1E0, 0x8, 0x238, 0x0, 0x10, 0x10, 0x0, 0x20, 0x58, 0x28 }
#define HP_LIMIT_OFFSETS    { 0x28, 0x38, 0x1E0, 0x8, 0x238, 0x0, 0x10, 0x10, 0x0, 0x20, 0x58, 0x2C }
#define DP_VALUE_OFFSETS    { 0x28, 0x38, 0x1E0, 0x8, 0x238, 0x0, 0x10, 0x8, 0x20, 0x58, 0x30 }
#define DP_LIMIT_OFFSETS    { 0x28, 0x38, 0x1E0, 0x8, 0x238, 0x0, 0x10, 0x10, 0x0, 0x20, 0x58, 0x34 }
#define COINS_OFFSETS       { 0x51050 }

#define SFX_GAME_START      "qrc:/audio/assets/yhn0.wav"
#define SFX_HP_LOCK         "qrc:/audio/assets/yhn1.wav"
#define SFX_DP_LOCK         "qrc:/audio/assets/yhn2.wav"
#define SFX_UN_LOCK         "qrc:/audio/assets/yhn3.wav"
#define SFX_COINS           "qrc:/audio/assets/sfx_gold.wav"

#include <Windows.h>
#include <QFileDialog>
#include <QString>
#include <initializer_list>
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <QPoint>
#include <QPainterPath>
#include <QMediaPlayer>

QT_BEGIN_NAMESPACE

namespace Ui
{
    class YohaneWindow;
}

QT_END_NAMESPACE

struct PlayerPoint
{
    int value = -1;
    int limit = -1;
    bool locked = false;
};

struct GameData
{
    PlayerPoint hp;
    PlayerPoint dp;
    long long coins = -1;
};

struct AddrList
{
    DWORD64 hp_value = 0;
    DWORD64 dp_value = 0;
    DWORD64 hp_limit = 0;
    DWORD64 dp_limit = 0;
    DWORD64 coins = 0;
};

// ReSharper disable once CppClassCanBeFinal
class YohaneWindow : public QWidget
{
    Q_OBJECT

public:
    explicit YohaneWindow(QWidget *parent = nullptr);

    ~YohaneWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void checkGameStatus();
    void on_hp_locker_clicked() const;
    void on_dp_locker_clicked() const;
    void on_coin_adder_clicked() const;

private:
    static DWORD64 getModuleBaseAddress(DWORD processID, const wchar_t* moduleName);
    void getModuleBasePrefix();
    [[nodiscard]] DWORD64 getPointer(DWORD64 g_nBaseAddr, std::initializer_list<DWORD> offsets) const;
    void readGameData();
    void playerPointDataCheck(DWORD64& addr, int& data, std::initializer_list<DWORD> offsets, bool locker) const;
    void coinsDataCheck() const;
    void updateDisplayData() const;

    static std::string formatNumber(const int n)
    {
        if (n < 10) return "00" + std::to_string(n);
        if (n < 100) return "0" + std::to_string(n);
        return std::to_string(n);
    }

    static bool isInDraggableArea(const QPoint &pos);
    void applyShadow();
    void playAudio(const QUrl &src) const;

    bool isGameRunning = false;
    HANDLE g_hProcess{};
    DWORD64 baseAddress{};
    DWORD64 moduleBasePrefix{};
    GameData *gameData;
    AddrList *addrList;
    QTimer *timer;
    QMediaPlayer *media_player;
    QAudioOutput *audio_output;

    Ui::YohaneWindow *ui;

    bool m_dragging = false;
    QPointF m_dragPosition;
    QGraphicsDropShadowEffect *windowShadow = nullptr;
};


#endif //YOHANEWINDOW_H
