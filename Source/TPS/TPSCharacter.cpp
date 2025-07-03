#include "TPSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Curves/CurveFloat.h"
#include "TPS.h"

ATPSCharacter::ATPSCharacter()
{
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;


	CursorToWorld = CreateDefaultSubobject<UDecalComponent>("CursorToWorld");
	CursorToWorld->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UMaterial> DecalMaterialAsset(TEXT("Material'/Game/Blueprint/Character/M_Cursor_Decal.M_Cursor_Decal'"));
	if (DecalMaterialAsset.Succeeded())
	{
		CursorToWorld->SetDecalMaterial(DecalMaterialAsset.Object);
	}
	CursorToWorld->DecalSize = FVector(16.0f, 32.0f, 32.0f);
	CursorToWorld->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f).Quaternion());

	// Stamina
	StaminaSettings.MaxStamina = 100.0f;
	StaminaSettings.SprintStaminaDrain = 20.0f;
	StaminaSettings.StaminaRecoveryRate = 15.0f;
	StaminaSettings.StaminaRecoveryDelay = 2.0f;
	StaminaSettings.MinSprintStamina = 15.0f;

	CurrentStamina = StaminaSettings.MaxStamina;
	StaminaRecoveryTimer = 0.0f;
	bIsSprintingInternal = false;
	CurrentStaminaState = EStaminaState::Normal;
	ForwardSprintThreshold = 0.7f;

	
	SprintTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SprintTimeline"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentStamina = StaminaSettings.MaxStamina;
	UpdateStaminaState();
}

void ATPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	
	if (CursorToWorld != nullptr)
	{
		if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
		{
			if (UWorld* World = GetWorld())
			{
				FHitResult HitResult;
				FCollisionQueryParams Params(NAME_None, FCollisionQueryParams::GetUnknownStatId());
				FVector StartLocation = TopDownCameraComponent->GetComponentLocation();
				FVector EndLocation = TopDownCameraComponent->GetComponentRotation().Vector() * 2000.0f;
				Params.AddIgnoredActor(this);
				World->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, Params);
				FQuat SurfaceRotation = HitResult.ImpactNormal.ToOrientationRotator().Quaternion();
				CursorToWorld->SetWorldLocationAndRotation(HitResult.Location, SurfaceRotation);
			}
		}
		else if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FHitResult TraceHitResult;
			PC->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
			FVector CursorFV = TraceHitResult.ImpactNormal;
			FRotator CursorR = CursorFV.Rotation();
			CursorToWorld->SetWorldLocation(TraceHitResult.Location);
			CursorToWorld->SetWorldRotation(CursorR);
		}
	}

	MovementTick(DeltaSeconds);
	UpdateStamina(DeltaSeconds);
}

void ATPSCharacter::SetupPlayerInputComponent(UInputComponent* NewInputComponent)
{
	Super::SetupPlayerInputComponent(NewInputComponent);

	NewInputComponent->BindAxis(TEXT("MoveForward"), this, &ATPSCharacter::InputAxisX);
	NewInputComponent->BindAxis(TEXT("MoveRight"), this, &ATPSCharacter::InputAxisY);
	NewInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ATPSCharacter::SprintPressed);
	NewInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ATPSCharacter::SprintReleased);
}

void ATPSCharacter::InputAxisY(float Value)
{
	AxisY = Value;
}

void ATPSCharacter::InputAxisX(float Value)
{
	AxisX = Value;
}

void ATPSCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);

	APlayerController* myController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (myController)
	{
		FHitResult ResultHit;
		myController->GetHitResultUnderCursorByChannel(ETraceTypeQuery::TraceTypeQuery6, false, ResultHit);

		float FindRotaterResultYaw = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), ResultHit.Location).Yaw;
		SetActorRotation(FQuat(FRotator(0.0f, FindRotaterResultYaw, 0.0f)));
	}
}

void ATPSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;

	
	switch (MovementState)
	{
	case EMovementState::Aim_State:
		ResSpeed = MovementSpeedInfo.AimSpeedNormal;
		break;
	case EMovementState::AimWalk_State:
		ResSpeed = MovementSpeedInfo.AimSpeedWalk;
		break;
	case EMovementState::Walk_State:
		ResSpeed = MovementSpeedInfo.WalkSpeedNormal;
		break;
	case EMovementState::Run_State:
		ResSpeed = MovementSpeedInfo.RunSpeedNormal;
		break;
	case EMovementState::SprintRun_State:
		ResSpeed = MovementSpeedInfo.SprintRunSpeedRun;
		break;
	default:
		break;
	}


	float SpeedModifier = GetSpeedModifier();
	ResSpeed *= SpeedModifier;
	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void ATPSCharacter::ChangeMovementState()
{
	
	if (SprintRunEnabled && (!CanSprint() || !IsMovingForward()))
	{
		SprintRunEnabled = false;
		StopSprint();
	}

	
	if (!WalkEnabled && !SprintRunEnabled && !AimEnabled)
	{
		MovementState = EMovementState::Run_State;
		StopSprint();
	}
	else
	{
		if (SprintRunEnabled && IsMovingForward())
		{
			WalkEnabled = false;
			AimEnabled = false;
			MovementState = EMovementState::SprintRun_State;
			StartSprint();
		}
		else if (WalkEnabled && !SprintRunEnabled && AimEnabled)
		{
			MovementState = EMovementState::AimWalk_State;
			StopSprint();
		}
		else if (WalkEnabled && !SprintRunEnabled && !AimEnabled)
		{
			MovementState = EMovementState::Walk_State;
			StopSprint();
		}
		else if (!WalkEnabled && !SprintRunEnabled && AimEnabled)
		{
			MovementState = EMovementState::Aim_State;
			StopSprint();
		}
		else
		{
			MovementState = EMovementState::Run_State;
			StopSprint();
		}
	}

	CharacterUpdate();
}

// Stamina system
void ATPSCharacter::UpdateStamina(float DeltaTime)
{
	bool bIsMoving = (FMath::Abs(AxisX) > 0.1f || FMath::Abs(AxisY) > 0.1f);

	if (bIsSprintingInternal && bIsMoving && IsMovingForward())
	{
		
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - StaminaSettings.SprintStaminaDrain * DeltaTime);
		StaminaRecoveryTimer = 0.0f;

		if (CurrentStamina <= 0.0f)
		{
			SprintRunEnabled = false;
			ChangeMovementState();
		}
	}
	else if (bIsSprintingInternal && bIsMoving && !IsMovingForward())
	{
		SprintRunEnabled = false;
		ChangeMovementState();
	}
	else
	{
	
		StaminaRecoveryTimer += DeltaTime;

		if (StaminaRecoveryTimer >= StaminaSettings.StaminaRecoveryDelay)
		{
			CurrentStamina = FMath::Min(StaminaSettings.MaxStamina,
				CurrentStamina + StaminaSettings.StaminaRecoveryRate * DeltaTime);
		}
	}

	UpdateStaminaState();
	OnStaminaChanged.Broadcast(CurrentStamina);
}

bool ATPSCharacter::CanSprint() const
{
	return CurrentStamina >= StaminaSettings.MinSprintStamina;
}

bool ATPSCharacter::IsMovingForward() const
{
	if (FMath::Abs(AxisX) < 0.1f && FMath::Abs(AxisY) < 0.1f)
	{
		return false;
	}

	FVector InputDirection = FVector(AxisX, AxisY, 0.0f).GetSafeNormal();
	FVector ForwardVector = GetActorForwardVector();
	float DotProduct = FVector::DotProduct(InputDirection, ForwardVector);

	return DotProduct >= ForwardSprintThreshold;
}

void ATPSCharacter::StartSprint()
{
	if (CanSprint())
	{
		bIsSprintingInternal = true;
		StaminaRecoveryTimer = 0.0f;
	}
	else
	{
		bIsSprintingInternal = false;
	}
}

void ATPSCharacter::StopSprint()
{
	if (bIsSprintingInternal)
	{
		bIsSprintingInternal = false;
	}
}

float ATPSCharacter::GetStaminaPercentage() const
{
	return (StaminaSettings.MaxStamina > 0.0f) ? (CurrentStamina / StaminaSettings.MaxStamina) : 0.0f;
}

void ATPSCharacter::UpdateStaminaState()
{
	EStaminaState NewState = EStaminaState::Normal;
	float StaminaPercentage = GetStaminaPercentage() * 100.0f;

	if (StaminaPercentage <= StaminaSettings.ExhaustedThreshold)
	{
		NewState = EStaminaState::Exhausted;
	}
	else if (StaminaPercentage <= StaminaSettings.TiredThreshold)
	{
		NewState = EStaminaState::Tired;
	}

	if (NewState != CurrentStaminaState)
	{
		CurrentStaminaState = NewState;
		OnStaminaStateChanged.Broadcast(CurrentStaminaState);
		CharacterUpdate();
	}
}

float ATPSCharacter::GetSpeedModifier() const
{
	switch (CurrentStaminaState)
	{
	case EStaminaState::Tired:
		return MovementSpeedInfo.TiredSpeedModifier;
	case EStaminaState::Exhausted:
		return MovementSpeedInfo.ExhaustedSpeedModifier;
	default:
		return 1.0f;
	}
}


void ATPSCharacter::SprintPressed()
{
	if (CanSprint() && IsMovingForward())
	{
		SprintRunEnabled = true;
		ChangeMovementState();
	}
}

void ATPSCharacter::SprintReleased()
{
	SprintRunEnabled = false;
	ChangeMovementState();
}