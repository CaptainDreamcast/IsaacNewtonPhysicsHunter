#include "gamescreen.h"

#include "storyscreen.h"

static struct 
{
    int mLevel = 0;
    int mGameTicks = 0;
    CollisionListData* mTileCollisionList;
    CollisionListData* mPlayerCollisionList;
    CollisionListData* mAppleCollisionList;
    CollisionListData* mEnemyCollisionList;
} gGameScreenData;

class GameScreen
{
    public:
    GameScreen() {
        load();
        //activateCollisionHandlerDebugMode();
        setVolume(0.2);
        setSoundEffectVolume(0.5);
        streamMusicFile("game/GAME.ogg");
    }

    MugenSpriteFile mSprites;
    MugenAnimations mAnimations;
    MugenSounds mSounds;

    void loadFiles() {
        mSprites = loadMugenSpriteFileWithoutPalette("game/GAME.sff");
        mAnimations = loadMugenAnimationFile("game/GAME.air");
        mSounds = loadMugenSoundFile("game/GAME.snd");
    }

    void load() {
        loadFiles();
        loadGame();
    }

    void loadGame()
    {
        loadCollisions();
        loadTiles();
        loadPlayer();
        loadEnemy();
        loadApple();
        loadTimer();
    }

    void loadCollisions() {
        gGameScreenData.mPlayerCollisionList = addCollisionListToHandler();
        gGameScreenData.mAppleCollisionList = addCollisionListToHandler();
        gGameScreenData.mTileCollisionList = addCollisionListToHandler();
        gGameScreenData.mEnemyCollisionList = addCollisionListToHandler();
        addCollisionHandlerCheck(gGameScreenData.mPlayerCollisionList, gGameScreenData.mTileCollisionList);
        addCollisionHandlerCheck(gGameScreenData.mAppleCollisionList, gGameScreenData.mTileCollisionList);
        addCollisionHandlerCheck(gGameScreenData.mEnemyCollisionList, gGameScreenData.mTileCollisionList);
        addCollisionHandlerCheck(gGameScreenData.mPlayerCollisionList, gGameScreenData.mAppleCollisionList);
    }

    void update() {
        updatePlayer();
        //updateApple();
        updateTiles();
        updateTimer();
        updateReset();
    }

    void updateReset() {
        if (hasPressedKeyboardKeyFlank(KEYBOARD_R_PRISM))
        {
            setNewScreen(getGameScreen());
        }
    }

    // Timer
    int mSpeedrunTimerText;
    void loadTimer() {
        mSpeedrunTimerText = addMugenTextMugenStyle("TIME: ", Vector3D(10, 10, 40), Vector3DI(2, 0, 1));
        updateTimer();
    }

    void updateTimer() {
        if (mIsWinning) return;
        if (mIsLosing) return;
        gGameScreenData.mGameTicks++;
        int seconds = gGameScreenData.mGameTicks / 60;
        changeMugenText(mSpeedrunTimerText, (std::string("TIME: ") + std::to_string(seconds) + " SECONDS").c_str());
    }

    // Tiles
    enum class RotationDirection {
        UP,
        DOWN,
        LEFT,
        RIGHT
    };
    RotationDirection mActiveRotation = RotationDirection::UP;

    struct Tile {
        int mTileID = -1;
        int mCollisionID = -1;
    };
    static constexpr auto TILE_COUNT_X = 240 / 16;
    static constexpr auto TILE_COUNT_Y = 240 / 16;
    Tile mTiles[TILE_COUNT_Y][TILE_COUNT_X];
    void loadTiles() {
        for (int y = 0; y < TILE_COUNT_Y; y++) {
            for (int x = 0; x < TILE_COUNT_X; x++) {
                mTiles[y][x].mTileID = -1;
            }
        }

        setPlayerStartPosition(4, 4);
        setAppleStartPosition(8, 4);
        setEnemyStartPosition(-20, 20);

        loadLevelFromFile();
    }

    void loadLevelFromFile() {
        const auto fileName = std::string("game/LEVEL") + std::to_string(gGameScreenData.mLevel) +  ".txt";
        auto b = fileToBuffer(fileName.c_str());
        auto p = getBufferPointer(b);
        for(int i = 0; i < TILE_COUNT_Y; i++) {
            const auto line = readStringFromTextStreamBufferPointer(&p);
            assert(line.size() == TILE_COUNT_X);
            for(int j = 0; j < TILE_COUNT_X; j++) {
                if(line[j] == 'T') {
                    setTile(j, i);
                } else if (line[j] == 'P') {
                    setPlayerStartPosition(j, i);
                } else if (line[j] == 'A') {
                    setAppleStartPosition(j, i);
                }
                else if (line[j] == 'E') {
                    setEnemyStartPosition(j, i);
                }
            }
        }
    }

    void setPlayerStartPosition(int x, int y) {
        mPlayerStartTile = { x, y };
    }

    void setAppleStartPosition(int x, int y) {
        mAppleStartTile = { x, y };
    }

    void setEnemyStartPosition(int x, int y) {
        mEnemyStartTile = { x, y };
    }

    Vector2D getTilePosition(int x, int y) {
        return { double(x * 16), double(y * 16) };
    }

    Vector2DI getTile(const Vector2D& position) {
        return { (int)position.x / 16, (int)position.y / 16 };
    }

    void setTile(int x, int y) {
        mTiles[y][x].mTileID = addBlitzEntity(getTilePosition(x, y).xyz(1));
        addBlitzMugenAnimationComponent(mTiles[y][x].mTileID, &mSprites, &mAnimations, 10);
        addBlitzCollisionComponent(mTiles[y][x].mTileID);
        mTiles[y][x].mCollisionID = addBlitzCollisionRect(mTiles[y][x].mTileID, gGameScreenData.mTileCollisionList, makeCollisionRect(Vector2D(0, 0), Vector2D(16, 16)));
        setBlitzCollisionSolid(mTiles[y][x].mTileID, mTiles[y][x].mCollisionID, 0);
    }

    void updateTiles() {}

    // Losing
    int isPlayerOutsideScreen() {
        auto position = getBlitzEntityPosition(mPlayerEntity);
        return position.x < -20 || position.x > 260 || position.y < -20 || position.y > 260;
    }

    int isAppleOutsideScreen() {
        auto position = getBlitzEntityPosition(mAppleEntity);
        return position.x < -20 || position.x > 260 || position.y < -20 || position.y > 260;
    }

    int mIsLosing = 0;
    int mLosingTicks = 0;
    MugenAnimationHandlerElement* mLosingScreenAnimation = nullptr;
    void updatePlayerLosing() {
        updatePlayerLosingStart();
        updatePlayerLosingProgression();
    }

    void updatePlayerLosingStart() {
        if(mIsLosing) return;

        if(isPlayerOutsideScreen() || isAppleOutsideScreen()) {
            stopStreamingMusicFile();
            tryPlayMugenSound(&mSounds, 2, 1);
            mIsLosing = 1;
            mLosingScreenAnimation = addMugenAnimation(getMugenAnimation(&mAnimations, 51), &mSprites, Vector3D(0, 0, 50));
            mLosingTicks = 0;
            changeBlitzMugenAnimationIfDifferent(mPlayerEntity, 20);
        }
    }

    void updatePlayerLosingProgression() {
        if(!mIsLosing) return;

        mLosingTicks++;
        if(hasPressedAFlank()) {
            setNewScreen(getGameScreen());
        }
    }

    // Winning
    int isPlayerCloseToApple() {
        return (vecLength(getBlitzEntityPosition(mPlayerEntity).xy() - getBlitzEntityPosition(mAppleEntity).xy()) < 5);
    }

    int mIsWinning = 0;
    int mWinningTicks = 0;
    MugenAnimationHandlerElement* mWinningScreenAnimation = nullptr;
    void updatePlayerWinning() {
        updatePlayerWinningStart();
        updatePlayerWinningProgression();
    }

    void updatePlayerWinningStart() {
        if(mIsWinning) return;

        if(isPlayerCloseToApple()) {
            stopStreamingMusicFile();
            tryPlayMugenSound(&mSounds, 2, 0);
            mIsWinning = 1;
            mWinningScreenAnimation = addMugenAnimation(getMugenAnimation(&mAnimations, 50), &mSprites, Vector3D(0, 0, 50));
            mWinningTicks = 0;
            changeBlitzMugenAnimationIfDifferent(mPlayerEntity, 20);
        }
    }

    void updatePlayerWinningProgression() {
        if(!mIsWinning) return;

        mWinningTicks++;
        if(hasPressedAFlank()) {
            if(gGameScreenData.mLevel == 2)
            {
                setCurrentStoryDefinitionFile("game/OUTRO.def", 0);
                setNewScreen(getStoryScreen());
            }
            else{
                gGameScreenData.mLevel++;
                setNewScreen(getGameScreen());
            }
        }     
    }

    // Player
    Vector2DI mPlayerStartTile = { 0, 0 };
    int mPlayerEntity = -1;
    int mPlayerCollisionID = -1;
    void loadPlayer() {
        mPlayerEntity = addBlitzEntity(getTilePosition(mPlayerStartTile.x, mPlayerStartTile.y).xyz(10));
        addBlitzMugenAnimationComponent(mPlayerEntity, &mSprites, &mAnimations, 20);
        addBlitzCollisionComponent(mPlayerEntity);
        mPlayerCollisionID = addBlitzCollisionRect(mPlayerEntity, gGameScreenData.mPlayerCollisionList, makeCollisionRect(Vector2D(-8, -32), Vector2D(8, 0)));
        setBlitzCollisionSolid(mPlayerEntity, mPlayerCollisionID, 1);
        addBlitzPhysicsComponent(mPlayerEntity);
        setBlitzPhysicsGravity(mPlayerEntity, Acceleration(0, 0.1, 0));
    }

    void updatePlayer()
    {
        updatePlayerMovement();
        updatePlayerJumping();
        updatePlayerRotating();
        updatePlayerLosing();
        updatePlayerWinning();
    }

    int isRotating = 0;
    double rotationStart = 0.0;
    double rotationTarget = 0.0;
    void updatePlayerRotating() {
        updatePlayerRotatingStart();
        updatePlayerRotatingProgression();
    }

    void updatePlayerRotatingStart() {
        if (isRotating) return;
        if (mIsWinning) return;
        if (mIsLosing) return;

        if (hasPressedLFlank()) {
            isRotating = 1;
            rotationTarget = rotationStart + (-M_PI / 2);
        }
        else if (hasPressedRFlank()) {
            isRotating = 1;
            rotationTarget = rotationStart + (M_PI / 2);
        }

        if(isRotating) {
            setBlitzPhysicsVelocityY(mPlayerEntity, 0);
            setBlitzPhysicsVelocityY(mAppleEntity, 0);
            setBlitzPhysicsVelocityY(mEnemyEntity, 0);
            setBlitzPhysicsGravity(mPlayerEntity, Acceleration(0, 0.0, 0));
            setBlitzPhysicsGravity(mAppleEntity, Acceleration(0, 0.0, 0));
            tryPlayMugenSoundAdvanced(&mSounds, 1, 1, 0.1);
            disableAllGameCollisions();
        }
    }

    void disableAllGameCollisions() {
        removeBlitzCollision(mPlayerEntity, mPlayerCollisionID);
        mPlayerCollisionID = -1;
        removeBlitzCollision(mAppleEntity, mAppleCollisionID);
        mAppleCollisionID = -1;
        removeBlitzCollision(mEnemyEntity, mEnemyCollisionID);
        mEnemyCollisionID = -1;
        for (int y = 0; y < TILE_COUNT_Y; y++) {
            for (int x = 0; x < TILE_COUNT_X; x++) {
                if (mTiles[y][x].mTileID != -1)
                {
                    removeBlitzCollision(mTiles[y][x].mTileID, mTiles[y][x].mCollisionID);
                    mTiles[y][x].mCollisionID = -1;
                }
            }
        }
    }

    CollisionRect getRotatedTileCollisionRect() {
        const auto vec1 = Vector2D(0, 0);
        const auto vec2 = vecRotateZ2D(Vector2D(16, 16), rotationStart);
        return makeCollisionRect(Vector2D(min(vec1.x, vec2.x), min(vec1.y, vec2.y)), Vector2D(max(vec1.x, vec2.x), max(vec1.y, vec2.y)));
    }

    void enableAllGameCollisions() {
        mPlayerCollisionID = addBlitzCollisionRect(mPlayerEntity, gGameScreenData.mPlayerCollisionList, makeCollisionRect(Vector2D(-8, -32), Vector2D(8, 0)));
        setBlitzCollisionSolid(mPlayerEntity, mPlayerCollisionID, 1);
        mAppleCollisionID =  addBlitzCollisionRect(mAppleEntity, gGameScreenData.mAppleCollisionList, makeCollisionRect(Vector2D(-9, -19), Vector2D(9, 0)));
        setBlitzCollisionSolid(mAppleEntity, mAppleCollisionID, 1);
        mEnemyCollisionID =  addBlitzCollisionRect(mEnemyEntity, gGameScreenData.mEnemyCollisionList, makeCollisionRect(Vector2D(-10, -35), Vector2D(10, 0)));
        setBlitzCollisionSolid(mEnemyEntity, mEnemyCollisionID, 1);
        for (int y = 0; y < TILE_COUNT_Y; y++) {
            for (int x = 0; x < TILE_COUNT_X; x++) {
                if (mTiles[y][x].mTileID != -1)
                {
                    mTiles[y][x].mCollisionID = addBlitzCollisionRect(mTiles[y][x].mTileID, gGameScreenData.mTileCollisionList, getRotatedTileCollisionRect());
                    setBlitzCollisionSolid(mTiles[y][x].mTileID, mTiles[y][x].mCollisionID, 0);
                }
            }
        }
    }

    void updateAllTileRotations() {
        // rotate all Tiles around center of tileset
        {
            Vector2D center = { 120, 120 };
            for (int y = 0; y < TILE_COUNT_Y; y++) {
                for (int x = 0; x < TILE_COUNT_X; x++) {
                    if (mTiles[y][x].mTileID != -1)
                    {
                        Vector2D position = getTilePosition(x, y);
                        Vector2D relativePosition = position - center;
                        Vector2D rotatedPosition = vecRotateZ2D(relativePosition, rotationStart);
                        Vector2D newPosition = rotatedPosition + center;
                        setBlitzEntityPosition(mTiles[y][x].mTileID, newPosition.xyz(1));
                        setBlitzMugenAnimationAngle(mTiles[y][x].mTileID, -rotationStart);
                    }
                }
            }
        }

        // rotate player around center of tileset
        {
            // Vector2D center = { 120, 120 };
            // Vector2D position = getBlitzEntityPosition(mPlayerEntity).xy();
            // Vector2D relativePosition = position - center;
            // Vector2D rotatedPosition = vecRotateZ2D(relativePosition, rotationStart);
            // Vector2D newPosition = rotatedPosition + center;
            // setBlitzEntityPosition(mPlayerEntity, newPosition.xyz(10));
        }
    }

    void updatePlayerRotatingProgression() {
        if (!isRotating) return;

        double delta = 0.1;
        if (rotationStart < rotationTarget) {
            rotationStart += delta;
            if (rotationStart >= rotationTarget) {
                rotationStart = rotationTarget;
                isRotating = 0;
            }
        }
        else {
            rotationStart -= delta;
            if (rotationStart <= rotationTarget) {
                rotationStart = rotationTarget;
                isRotating = 0;
            }
        }

        updateAllTileRotations();

        if(!isRotating) {
            enableAllGameCollisions();
            setBlitzPhysicsGravity(mPlayerEntity, Acceleration(0, 0.1, 0));
            setBlitzPhysicsGravity(mAppleEntity, Acceleration(0, 0.1, 0));
            setBlitzPhysicsGravity(mEnemyEntity, Acceleration(0, 0.1, 0));
        }
    }

    int isJumping = 0;
    void updatePlayerMovement() {
        if (isRotating) return;
        if (mIsWinning) return;
        if (mIsLosing) return;

        bool isMoving = false;
        static constexpr double movementSpeed = 2;
        double movementSpeedX = 0;
        if(hasPressedLeft()) {
            movementSpeedX = -movementSpeed;
            isMoving = true;
            setBlitzMugenAnimationFaceDirection(mPlayerEntity, 0);
        } else if(hasPressedRight()) {
            movementSpeedX = movementSpeed;
            isMoving = true;
            setBlitzMugenAnimationFaceDirection(mPlayerEntity, 1);
        }
        addBlitzEntityPositionX(mPlayerEntity, movementSpeedX);

        if(!isJumping) {
            if(isMoving)
            {
                changeBlitzMugenAnimationIfDifferent(mPlayerEntity, 21);
            }
            else
            {
                changeBlitzMugenAnimationIfDifferent(mPlayerEntity, 20);
            }
        }
    }

    void updatePlayerJumping() {
        if (isRotating) return;
        if (mIsWinning) return;
        if (mIsLosing) return;
        updatePlayerJumpingStart();
        updatePlayerFallingStart();
        updatePlayerLanding();
    }

    int jumpStartFrames = 0;
    void updatePlayerJumpingStart() {
        if (isJumping) return;

        if(hasPressedAFlank()) {
            tryPlayMugenSoundAdvanced(&mSounds, 1, 0, 0.1);
            isJumping = 1;
            setBlitzPhysicsVelocityY(mPlayerEntity, -4);
            jumpStartFrames = 0;
        }
    }

    bool isFalling() {
        return !hasBlitzCollidedBottom(mPlayerEntity);
    }

    void updatePlayerFallingStart() {
        if (!isJumping) return;
        if(!isFalling() && jumpStartFrames > 2) return;
        isJumping = 1;
        jumpStartFrames++;
        auto velY = getBlitzPhysicsVelocityY(mPlayerEntity);
        velY += 0.1;
        setBlitzPhysicsVelocityY(mPlayerEntity, velY);
    }

    void updatePlayerLanding() {
        if (!isJumping) return;
        changeBlitzMugenAnimation(mPlayerEntity, 22);
        if (!isFalling() && jumpStartFrames > 2) {
            isJumping = 0;
            jumpStartFrames = 0;
            setBlitzPhysicsVelocityY(mPlayerEntity, 0.0);
        }
    }

    // Apple
    Vector2DI mAppleStartTile = { 0, 0 };
    int mAppleEntity = -1;
    int mAppleCollisionID = -1;
    void loadApple() {
        mAppleEntity = addBlitzEntity(getTilePosition(mAppleStartTile.x, mAppleStartTile.y).xyz(20));
        addBlitzMugenAnimationComponent(mAppleEntity, &mSprites, &mAnimations, 40);
        addBlitzCollisionComponent(mAppleEntity);
        mAppleCollisionID = addBlitzCollisionRect(mAppleEntity, gGameScreenData.mAppleCollisionList, makeCollisionRect(Vector2D(-9, -19), Vector2D(9, 0)));
        setBlitzCollisionSolid(mAppleEntity, mAppleCollisionID, 1);
        addBlitzPhysicsComponent(mAppleEntity);
        setBlitzPhysicsGravity(mAppleEntity, Acceleration(0, 0.1, 0));
    }

    // Enemy
    Vector2DI mEnemyStartTile = { 0, 0 };
    int mEnemyEntity = -1;
    int mEnemyCollisionID = -1;

    void loadEnemy() {
        mEnemyEntity = addBlitzEntity(getTilePosition(mEnemyStartTile.x, mEnemyStartTile.y).xyz(30));
        addBlitzMugenAnimationComponent(mEnemyEntity, &mSprites, &mAnimations, 30);
        setBlitzMugenAnimationFaceDirection(mEnemyEntity, 0);
        addBlitzCollisionComponent(mEnemyEntity);
        mEnemyCollisionID = addBlitzCollisionRect(mEnemyEntity, gGameScreenData.mEnemyCollisionList, makeCollisionRect(Vector2D(-10, -35), Vector2D(10, 0)));
        setBlitzCollisionSolid(mEnemyEntity, mEnemyCollisionID, 1);
        addBlitzPhysicsComponent(mEnemyEntity);
        setBlitzPhysicsGravity(mEnemyEntity, Acceleration(0, 0.1, 0));
    }
};

EXPORT_SCREEN_CLASS(GameScreen);

void resetGame()
{
    gGameScreenData.mLevel = 0;
    gGameScreenData.mGameTicks = 0;
}