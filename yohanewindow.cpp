#include "yohanewindow.h"
#include "ui_YohaneWindow.h"

#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>

YohaneWindow::YohaneWindow(QWidget *parent) : QWidget(parent),
                                              gameData(new GameData),
                                              addrList(new AddrList),
                                              ui(new Ui::YohaneWindow)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    ui->setupUi(this);
    applyShadow();

    timer = new QTimer(this);
    timer->start(100);

    media_player = new QMediaPlayer;
    audio_output = new QAudioOutput;
    media_player->setAudioOutput(audio_output);
    audio_output->setVolume(100);

    if (!connect(timer, &QTimer::timeout, this, &YohaneWindow::checkGameStatus))
    {
        qWarning() << "Failed to connect QTimer signal to slot.";
    }
}

YohaneWindow::~YohaneWindow()
{
    delete ui;
    delete timer;
    delete media_player;
    delete audio_output;
    delete gameData;
    delete addrList;
    delete windowShadow;
}

DWORD64 YohaneWindow::getModuleBaseAddress(const DWORD processID, const wchar_t *moduleName)
{
    // ReSharper disable once CppLocalVariableMayBeConst
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(hSnapshot, &me32))
    {
        do
        {
            if (_wcsicmp(me32.szModule, moduleName) == 0)
            {
                CloseHandle(hSnapshot);
                return reinterpret_cast<DWORD64>(me32.modBaseAddr);
            }
        } while (Module32Next(hSnapshot, &me32));
    }

    CloseHandle(hSnapshot);
    return 0;
}

void YohaneWindow::getModuleBasePrefix()
{
    DWORD64 temp;

    if (!ReadProcessMemory(
        g_hProcess,
        reinterpret_cast<LPCVOID>(baseAddress + PLAYER_POINT_BASE_ADDR1),
        &temp,
        sizeof(DWORD64),
        nullptr))
    {
        const DWORD error = GetLastError();
        qDebug() << "!Failed to read memory. Error code: " << error;
    }

    moduleBasePrefix = temp * 0x100000000;
}

DWORD64 YohaneWindow::getPointer(const DWORD64 g_nBaseAddr, const std::initializer_list<DWORD> offsets) const
{
    auto currentAddress = reinterpret_cast<LPCVOID>(g_nBaseAddr);
    DWORD temp;

    for (const DWORD offset: offsets)
    {
        if (!ReadProcessMemory(g_hProcess, currentAddress, &temp, sizeof(DWORD), nullptr))
        {
            const DWORD error = GetLastError();
            qDebug() << "offset: " << offset;
            qDebug() << "currentAddress: " << currentAddress;
            qDebug() << "Failed to read memory. Error code: " << error;
            return 0;
        }
        currentAddress = reinterpret_cast<LPCVOID>(moduleBasePrefix + temp + offset);
        //qDebug() << currentAddress;
    }
    return reinterpret_cast<DWORD64>(currentAddress);
}


void YohaneWindow::checkGameStatus()
{
    // ReSharper disable once CppLocalVariableMayBeConst
    HWND hWnd = FindWindow(nullptr, TEXT("YOHANE THE PARHELION -BLAZE in the DEEPBLUE-"));
    if (nullptr == hWnd)
    {
        isGameRunning = false;
        ui->game_status_val->setText("未检测到游戏运行");
        readGameData();
        updateDisplayData();
        return;
    }

    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd, &dwProcessId);
    if(!isGameRunning)
    {
        g_hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, dwProcessId);
        if (nullptr == g_hProcess)
        {
            isGameRunning = false;
            ui->game_status_val->setText("未检测到游戏运行");
            readGameData();
            updateDisplayData();
            return;
        }
        isGameRunning = true;
        playAudio(QUrl(SFX_GAME_START));
        ui->game_status_val->setText("游戏正在运行");
        qDebug() << "游戏进程ID:" << dwProcessId;
        baseAddress = getModuleBaseAddress(dwProcessId, L"game.exe");
        qDebug() << "game.exe基址:" << baseAddress;
        getModuleBasePrefix();
        qDebug() << "属性地址前缀:" << moduleBasePrefix;
    }

    readGameData();
    updateDisplayData();
}

void YohaneWindow::readGameData()
{
    if (!isGameRunning)
    {
        delete gameData;
        gameData = new GameData;
        ui->hp_locker->setText("锁定");
        ui->dp_locker->setText("锁定");
        return;
    }

    playerPointDataCheck(addrList->hp_value, gameData->hp.value, HP_VALUE_OFFSETS, gameData->hp.locked);
    playerPointDataCheck(addrList->hp_limit, gameData->hp.limit, HP_LIMIT_OFFSETS, gameData->hp.locked);
    playerPointDataCheck(addrList->dp_value, gameData->dp.value, DP_VALUE_OFFSETS, gameData->dp.locked);
    playerPointDataCheck(addrList->dp_limit, gameData->dp.limit, DP_LIMIT_OFFSETS, gameData->dp.locked);
    coinsDataCheck();
}

void YohaneWindow::playerPointDataCheck(DWORD64& addr, int& data, const std::initializer_list<DWORD> offsets, const bool locker) const
{
    DWORD temp = 0;

    if(!addr || !ReadProcessMemory(g_hProcess, reinterpret_cast<LPCVOID>(addr), &temp, sizeof(DWORD), nullptr))
    {
        addr = getPointer(baseAddress + PLAYER_POINT_BASE_ADDR2, offsets);
        ReadProcessMemory(g_hProcess, reinterpret_cast<LPCVOID>(addr), &temp, sizeof(DWORD), nullptr);
    }

    if(!locker)
    {
        if(temp == 999 && data > 0)
        {
            WriteProcessMemory(g_hProcess, reinterpret_cast<LPVOID>(addr), &data,sizeof(DWORD),nullptr);
        }
        else { data = static_cast<int>(temp); }
    }
    else if(temp != 999)
    {
        temp = 999;
        WriteProcessMemory(g_hProcess, reinterpret_cast<LPVOID>(addr), &temp,sizeof(DWORD),nullptr);
    }
}

void YohaneWindow::coinsDataCheck() const
{
    DWORD temp = 0;

    if(!addrList->coins || !ReadProcessMemory(g_hProcess, reinterpret_cast<LPCVOID>(addrList->coins), &temp, sizeof(DWORD), nullptr))
    {
        addrList->coins = getPointer(baseAddress + PLAYER_COINS_BASE_ADDR, COINS_OFFSETS);
        ReadProcessMemory(g_hProcess, reinterpret_cast<LPCVOID>(addrList->coins), &temp, sizeof(DWORD), nullptr);
    }

    gameData->coins = temp;
}

void YohaneWindow::updateDisplayData() const
{
    const std::string hp_value = gameData->hp.value < 0 ? std::string("---") : formatNumber(gameData->hp.value);
    const std::string hp_limit = gameData->hp.limit < 0 ? std::string("---") : formatNumber(gameData->hp.limit);
    const std::string dp_value = gameData->dp.value < 0 ? std::string("---") : formatNumber(gameData->dp.value);
    const std::string dp_limit = gameData->dp.limit < 0 ? std::string("---") : formatNumber(gameData->dp.limit);

    const std::string hp_label = gameData->hp.locked ? "999/999" : std::format("{}/{}", hp_value, hp_limit);
    const std::string dp_label = gameData->dp.locked ? "999/999" : std::format("{}/{}", dp_value, dp_limit);
    const std::string coins = gameData->coins < 0 ? std::string("￥------") : "￥"+std::to_string(gameData->coins);

    ui->hp_val->setText(QString::fromStdString(hp_label));
    ui->dp_val->setText(QString::fromStdString(dp_label));
    ui->coin_val->setText(QString::fromStdString(coins));
}

void YohaneWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isInDraggableArea(event->pos()))
    {
        m_dragging = true;
        m_dragPosition = event->globalPosition() - frameGeometry().topLeft();
        event->accept();
    }
}

void YohaneWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
    {
        move((event->globalPosition() - m_dragPosition).toPoint());
        event->accept();
    }
}

void YohaneWindow::mouseReleaseEvent(QMouseEvent *event) { m_dragging = false; }

bool YohaneWindow::isInDraggableArea(const QPoint &pos) { return pos.y() < 50 && pos.y() > 20; }

void YohaneWindow::applyShadow()
{
    windowShadow = new QGraphicsDropShadowEffect(this);
    windowShadow->setBlurRadius(15);
    windowShadow->setColor(QColor(64, 64, 64));
    windowShadow->setOffset(0, 0);
    ui->mainFrame->setGraphicsEffect(windowShadow);
}

void YohaneWindow::playAudio(const QUrl &src) const
{
    media_player->setSource(src);
    media_player->play();
}

void YohaneWindow::on_hp_locker_clicked() const
{
    if(!isGameRunning) return;
    if(gameData->hp.locked)
    {
        gameData->hp.locked = false;
        ui->hp_locker->setText("锁定");
        playAudio(QUrl(SFX_UN_LOCK));
    }
    else
    {
        gameData->hp.locked = true;
        ui->hp_locker->setText("解锁");
        playAudio(QUrl(SFX_HP_LOCK));
    }
}

void YohaneWindow::on_dp_locker_clicked() const
{
    if(!isGameRunning) return;
    if(gameData->dp.locked)
    {
        gameData->dp.locked = false;
        ui->dp_locker->setText("锁定");
        playAudio(QUrl(SFX_UN_LOCK));
    }
    else
    {
        gameData->dp.locked = true;
        ui->dp_locker->setText("解锁");
        playAudio(QUrl(SFX_DP_LOCK));
    }
}

void YohaneWindow::on_coin_adder_clicked() const
{
    if(!isGameRunning) return;
    gameData->coins += 10000;
    playAudio(QUrl(SFX_COINS));
    WriteProcessMemory(g_hProcess, reinterpret_cast<LPVOID>(addrList->coins), &gameData->coins, sizeof(DWORD), nullptr);
}
