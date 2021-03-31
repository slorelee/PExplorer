#pragma once
class DesktopCommand
{
public:
    DesktopCommand();
    ~DesktopCommand();
    void Refresh();
    void SetIconSize(int size);
    void AutoArrange(int checked);
    void SnapToGrid(int checked);
    void ShowIcons(int checked);
};

