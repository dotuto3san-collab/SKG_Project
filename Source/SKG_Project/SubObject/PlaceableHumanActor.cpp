#include "PlaceableHumanActor.h"

void APlaceableHumanActor::SetHumanStatus(EHumanStatus NewStatus)
{
    if (HumanStatus == NewStatus)
    {
        return; // ä˘Ç…ìØÇ∂èÛë‘Ç»ÇÁâΩÇ‡ÇµÇ»Ç¢
    }

    if (NewStatus == EHumanStatus::Danger)
    {
        OriginalRotation = GetActorRotation();

        FRotator FallenRotation = OriginalRotation;
        FallenRotation.Pitch += 90.f; // ëOÇ…ì|ÇÍÇÈ

        SetActorRotation(FallenRotation);
    }
    else // NormalÇ…ñﬂÇ∑
    {
        SetActorRotation(OriginalRotation);
    }

    HumanStatus = NewStatus;
}
