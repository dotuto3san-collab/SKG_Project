#include "PlaceableHumanActor.h"

void APlaceableHumanActor::SetHumanStatus(EHumanStatus NewStatus)
{
    HumanStatus = NewStatus;

    // 将来ここに、危険状態になった時の処理(色変化・通知など)を追加していく想定
}
