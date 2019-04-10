#include "EstPlayerHUD.h"
#include "EstCore.h"
#include "EstPlayer.h"
#include "EstPlayerController.h"
#include "EstFirearmWeapon.h"
#include "EstFirearmAmunition.h"
#include "EstResourceComponent.h"
#include "EstHealthComponent.h"
#include "Runtime/Engine/Classes/Engine/Canvas.h"
#include "Kismet/KismetMathLibrary.h"
#include "EstPlayer.h"

void AEstPlayerHUD::BeginPlay()
{
	Super::BeginPlay();

	Player = Cast<AEstPlayer>(GetOwningPawn());
	if (Player.IsValid())
	{
		Player->OnTakeAnyDamage.AddDynamic(this, &AEstPlayerHUD::HandleDamage);
		Controller = Cast<AEstPlayerController>(Player->GetController());
	}
}

void AEstPlayerHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (Player.IsValid())
	{
		Player->OnTakePointDamage.RemoveAll(this);
	}
}

void AEstPlayerHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Player.IsValid())
	{
		return;
	}

	if (!Controller.IsValid())
	{
		return;
	}

	DrawDamageIndicators();
	DrawLoadingIndicator();

	LastCanvasSizeX = Canvas->SizeX;
	LastCanvasSizeY = Canvas->SizeY;
}

void AEstPlayerHUD::DrawLoadingIndicator()
{
	if (!bIsLoading)
	{
		return;
	}

	const FString LoadingLabel = LoadingLabelText.ToString();

	float LabelWidth;
	float LabelHeight;
	GetTextSize(LoadingLabel, LabelWidth, LabelHeight, LoadingLabelFont);

	const float BoxHeight = LabelHeight * 2.f;
	const float VerticalCenter = float(Canvas->SizeY) * .5f;
	const float HorizontalCenter = float(Canvas->SizeX) * .5f;

	DrawRect(FLinearColor::Black, 0.f, VerticalCenter - (BoxHeight * .5f), float(Canvas->SizeX), BoxHeight);
	DrawText(LoadingLabel, FLinearColor::White, HorizontalCenter - (LabelWidth * .5f), VerticalCenter - (LabelHeight * .5f), LoadingLabelFont);
}

void AEstPlayerHUD::DrawDamageIndicators()
{
	if (Player->HealthComponent->IsFrozen)
	{
		return;
	}

	if (Player->HealthComponent->IsDepleted())
	{
		DrawRect(DeathOverlayColor, 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
		return;
	}

	// Calculate the fade out
	const float FadeAlpha = 1.f - GetWorld()->TimeSince(LastDamageTime) / DamageIndicatorFadeTime;
	if (FadeAlpha < 0.f)
	{
		return;
	}

	if (LastDamageType == nullptr)
	{
		return;
	}

	const FLinearColor* Color = DamageTypeMapping.Find(LastDamageType->GetClass());
	if (Color == nullptr)
	{
		return;
	}

	const FLinearColor DamageColor = *Color;

	const FVector CameraForwardVector = Player->Camera->GetForwardVector();
	const float DotProduct = FVector::DotProduct(CameraForwardVector, LastDamageLocation);

	const float HalfX = Canvas->SizeX * .5f;
	const float HalfY = Canvas->SizeY * .5f;

	FVector Projected = Project(LastDamageLocation);

	Projected.X = FMath::Clamp(Projected.X, 0.f, float(Canvas->SizeX));
	Projected.Y = FMath::Clamp(Projected.Y, 0.f, float(Canvas->SizeY));

	const bool bIsFacing = Projected.Z > 0.f;
	if (!bIsFacing)
	{
		Projected.X -= Canvas->SizeX;
		Projected.Y -= Canvas->SizeY;
	}

	Projected.X = FMath::Abs(Projected.X);
	Projected.Y = FMath::Abs(Projected.Y);

	// Left
	const float LeftAlpha = LastDamageType->bCausedByWorld ? 1.f : 1.f - (FMath::Clamp(Projected.X, 0.f, HalfX) / HalfX);
	DrawTexture(DamageIndicator, 0, Canvas->SizeY, Canvas->SizeY, Canvas->SizeX * DamageIndicatorSize.X, 0, 0, 1, 1, DamageColor * LeftAlpha * FadeAlpha, BLEND_Additive, 1, false, 270.f);

	// Right
	const float RightAlpha = LastDamageType->bCausedByWorld ? 1.f : (FMath::Clamp(Projected.X, HalfX, (float)Canvas->SizeX) - HalfX) / HalfX;
	DrawTexture(DamageIndicator, Canvas->SizeX, 0, Canvas->SizeY, Canvas->SizeX * DamageIndicatorSize.X, 0, 0, 1, 1, DamageColor * RightAlpha * FadeAlpha, BLEND_Additive, 1, false, 90.f);

	// Top
	const float TopAlpha = LastDamageType->bCausedByWorld ? 1.f : 1.f - (FMath::Clamp(Projected.Y, 0.f, HalfY) / HalfY);
	DrawTexture(DamageIndicator, 0, 0, Canvas->SizeX, Canvas->SizeY * DamageIndicatorSize.Y, 0, 0, 1, 1, DamageColor * TopAlpha * FadeAlpha, BLEND_Additive, 1, false, 0);

	// Bottom
	const float BottomAlpha = LastDamageType->bCausedByWorld ? 1.f : (FMath::Clamp(Projected.Y, HalfY, (float)Canvas->SizeY) - HalfY) / HalfY;
	DrawTexture(DamageIndicator, Canvas->SizeX, Canvas->SizeY, Canvas->SizeX, Canvas->SizeY * DamageIndicatorSize.Y, 0, 0, 1, 1, DamageColor * BottomAlpha * FadeAlpha, BLEND_Additive, 1, false, 180.f);
}

void AEstPlayerHUD::HandleDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (DamagedActor->ActorHasTag(TAG_DEAD))
	{
		return;
	}

	LastDamageAmount = Damage;
	LastDamageLocation = DamageCauser == nullptr ? DamagedActor->GetActorLocation() : DamageCauser->GetActorLocation();
	LastDamageTime = GetWorld()->GetTimeSeconds();
	LastDamageType = DamageType;
}