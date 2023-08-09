//Created for 1.20.1, protocol 763

//Handshaking packets

#define HANDSHAKE 0x00 //Bound to Server during handshaking. https://wiki.vg/Protocol#Handshake
#define LEGACY_SERVER_LIST_PING 0xFE //Bound to Server during handshaking. https://wiki.vg/Protocol#Legacy_Server_List_Ping

//Status packets

#define STATUS_RESPONSE 0x00 //Bound to Client during status. https://wiki.vg/Protocol#Status_Response
#define PING_RESPONSE 0x01 //Bound to Client during status. https://wiki.vg/Protocol#Ping_Response
#define STATUS_REQUEST 0x00 //Bound to Server during status. https://wiki.vg/Protocol#Status_Request
#define PING_REQUEST 0x01 //Bound to Server during status. https://wiki.vg/Protocol#Ping_Request

//Login packets

#define DISCONNECT_LOGIN 0x00 //Bound to Client during login. https://wiki.vg/Protocol#Disconnect_(login)
#define ENCRYPTION_REQUEST 0x01 //Bound to Client during login. https://wiki.vg/Protocol#Encryption_Request
#define LOGIN_SUCCESS 0x02 //Bound to Client during login. https://wiki.vg/Protocol#Login_Success
#define SET_COMPRESSION 0x03 //Bound to Client during login. https://wiki.vg/Protocol#Set_Compression
#define LOGIN_PLUGIN_REQUEST 0x04 //Bound to Client during login. https://wiki.vg/Protocol#Login_Plugin_Request
#define LOGIN_START 0x00 //Bound to Server during login. https://wiki.vg/Protocol#Login_Start
#define ENCRYPTION_RESPONSE 0x01 //Bound to Server during login. https://wiki.vg/Protocol#Encryption_Response
#define LOGIN_PLUGIN_RESPONSE 0x02 //Bound to Server during login. https://wiki.vg/Protocol#Login_Plugin_Response

//Play packets

#define BUNDLE_DELIMITER 0x00 //Bound to Client during play. https://wiki.vg/Protocol#Bundle_Delimiter
#define SPAWN_ENTITY 0x01 //Bound to Client during play. https://wiki.vg/Protocol#Spawn_Entity
#define SPAWN_EXPERIENCE_ORB 0x02 //Bound to Client during play. https://wiki.vg/Protocol#Spawn_Experience_Orb
#define SPAWN_PLAYER 0x03 //Bound to Client during play. https://wiki.vg/Protocol#Spawn_Player
#define ENTITY_ANIMATION 0x04 //Bound to Client during play. https://wiki.vg/Protocol#Entity_Animation
#define AWARD_STATISTICS 0x05 //Bound to Client during play. https://wiki.vg/Protocol#Award_Statistics
#define ACKNOWLEDGE_BLOCK_CHANGE 0x06 //Bound to Client during play. https://wiki.vg/Protocol#Acknowledge_Block_Change
#define SET_BLOCK_DESTROY_STAGE 0x07 //Bound to Client during play. https://wiki.vg/Protocol#Set_Block_Destroy_Stage
#define BLOCK_ENTITY_DATA 0x08 //Bound to Client during play. https://wiki.vg/Protocol#Block_Entity_Data
#define BLOCK_ACTION 0x09 //Bound to Client during play. https://wiki.vg/Protocol#Block_Action
#define BLOCK_UPDATE 0x0A //Bound to Client during play. https://wiki.vg/Protocol#Block_Update
#define BOSS_BAR 0x0B //Bound to Client during play. https://wiki.vg/Protocol#Boss_Bar
#define CHANGE_DIFFICULTY 0x0C //Bound to Client during play. https://wiki.vg/Protocol#Change_Difficulty
#define CHUNK_BIOMES 0x0D //Bound to Client during play. https://wiki.vg/Protocol#Chunk_Biomes
#define CLEAR_TITLES 0x0E //Bound to Client during play. https://wiki.vg/Protocol#Clear_Titles
#define COMMAND_SUGGESTIONS_RESPONSE 0x0F //Bound to Client during play. https://wiki.vg/Protocol#Command_Suggestions_Response
#define COMMANDS 0x10 //Bound to Client during play. https://wiki.vg/Protocol#Commands
#define CLOSE_CONTAINER 0x11 //Bound to Client during play. https://wiki.vg/Protocol#Close_Container
#define SET_CONTAINER_CONTENT 0x12 //Bound to Client during play. https://wiki.vg/Protocol#Set_Container_Content
#define SET_CONTAINER_PROPERTY 0x13 //Bound to Client during play. https://wiki.vg/Protocol#Set_Container_Property
#define SET_CONTAINER_SLOT 0x14 //Bound to Client during play. https://wiki.vg/Protocol#Set_Container_Slot
#define SET_COOLDOWN 0x15 //Bound to Client during play. https://wiki.vg/Protocol#Set_Cooldown
#define CHAT_SUGGESTIONS 0x16 //Bound to Client during play. https://wiki.vg/Protocol#Chat_Suggestions
#define PLUGIN_MESSAGE 0x17 //Bound to Client during play. https://wiki.vg/Protocol#Plugin_Message
#define DAMAGE_EVENT 0x18 //Bound to Client during play. https://wiki.vg/Protocol#Damage_Event
#define DELETE_MESSAGE 0x19 //Bound to Client during play. https://wiki.vg/Protocol#Delete_Message
#define DISCONNECT_PLAY 0x1A //Bound to Client during play. https://wiki.vg/Protocol#Disconnect_(play)
#define DISGUISED_CHAT_MESSAGE 0x1B //Bound to Client during play. https://wiki.vg/Protocol#Disguised_Chat_Message
#define ENTITY_EVENT 0x1C //Bound to Client during play. https://wiki.vg/Protocol#Entity_Event
#define EXPLOSION 0x1D //Bound to Client during play. https://wiki.vg/Protocol#Explosion
#define UNLOAD_CHUNK 0x1E //Bound to Client during play. https://wiki.vg/Protocol#Unload_Chunk
#define GAME_EVENT 0x1F //Bound to Client during play. https://wiki.vg/Protocol#Game_Event
#define OPEN_HORSE_SCREEN 0x20 //Bound to Client during play. https://wiki.vg/Protocol#Open_Horse_Screen
#define HURT_ANIMATION 0x21 //Bound to Client during play. https://wiki.vg/Protocol#Hurt_Animation
#define INITIALIZE_WORLD_BORDER 0x22 //Bound to Client during play. https://wiki.vg/Protocol#Initialize_World_Border
#define KEEP_ALIVE 0x23 //Bound to Client during play. https://wiki.vg/Protocol#Keep_Alive
#define CHUNK_DATA_AND_UPDATE_LIGHT 0x24 //Bound to Client during play. https://wiki.vg/Protocol#Chunk_Data_and_Update_Light
#define WORLD_EVENT 0x25 //Bound to Client during play. https://wiki.vg/Protocol#World_Event
#define PARTICLE_2 0x26 //Bound to Client during play. https://wiki.vg/Protocol#Particle_2
#define UPDATE_LIGHT 0x27 //Bound to Client during play. https://wiki.vg/Protocol#Update_Light
#define LOGIN_PLAY 0x28 //Bound to Client during play. https://wiki.vg/Protocol#Login_(play)
#define MAP_DATA 0x29 //Bound to Client during play. https://wiki.vg/Protocol#Map_Data
#define MERCHANT_OFFERS 0x2A //Bound to Client during play. https://wiki.vg/Protocol#Merchant_Offers
#define UPDATE_ENTITY_POSITION 0x2B //Bound to Client during play. https://wiki.vg/Protocol#Update_Entity_Position
#define UPDATE_ENTITY_POSITION_AND_ROTATION 0x2C //Bound to Client during play. https://wiki.vg/Protocol#Update_Entity_Position_and_Rotation
#define UPDATE_ENTITY_ROTATION 0x2D //Bound to Client during play. https://wiki.vg/Protocol#Update_Entity_Rotation
#define MOVE_VEHICLE 0x2E //Bound to Client during play. https://wiki.vg/Protocol#Move_Vehicle
#define OPEN_BOOK 0x2F //Bound to Client during play. https://wiki.vg/Protocol#Open_Book
#define OPEN_SCREEN 0x30 //Bound to Client during play. https://wiki.vg/Protocol#Open_Screen
#define OPEN_SIGN_EDITOR 0x31 //Bound to Client during play. https://wiki.vg/Protocol#Open_Sign_Editor
#define PING_PLAY 0x32 //Bound to Client during play. https://wiki.vg/Protocol#Ping_(play)
#define PLACE_GHOST_RECIPE 0x33 //Bound to Client during play. https://wiki.vg/Protocol#Place_Ghost_Recipe
#define PLAYER_ABILITIES 0x34 //Bound to Client during play. https://wiki.vg/Protocol#Player_Abilities
#define PLAYER_CHAT_MESSAGE 0x35 //Bound to Client during play. https://wiki.vg/Protocol#Player_Chat_Message
#define END_COMBAT 0x36 //Bound to Client during play. https://wiki.vg/Protocol#End_Combat
#define ENTER_COMBAT 0x37 //Bound to Client during play. https://wiki.vg/Protocol#Enter_Combat
#define COMBAT_DEATH 0x38 //Bound to Client during play. https://wiki.vg/Protocol#Combat_Death
#define PLAYER_INFO_REMOVE 0x39 //Bound to Client during play. https://wiki.vg/Protocol#Player_Info_Remove
#define PLAYER_INFO_UPDATE 0x3A //Bound to Client during play. https://wiki.vg/Protocol#Player_Info_Update
#define LOOK_AT 0x3B //Bound to Client during play. https://wiki.vg/Protocol#Look_At
#define SYNCHRONIZE_PLAYER_POSITION 0x3C //Bound to Client during play. https://wiki.vg/Protocol#Synchronize_Player_Position
#define UPDATE_RECIPE_BOOK 0x3D //Bound to Client during play. https://wiki.vg/Protocol#Update_Recipe_Book
#define REMOVE_ENTITIES 0x3E //Bound to Client during play. https://wiki.vg/Protocol#Remove_Entities
#define REMOVE_ENTITY_EFFECT 0x3F //Bound to Client during play. https://wiki.vg/Protocol#Remove_Entity_Effect
#define RESOURCE_PACK 0x40 //Bound to Client during play. https://wiki.vg/Protocol#Resource_Pack
#define RESPAWN 0x41 //Bound to Client during play. https://wiki.vg/Protocol#Respawn
#define SET_HEAD_ROTATION 0x42 //Bound to Client during play. https://wiki.vg/Protocol#Set_Head_Rotation
#define UPDATE_SECTION_BLOCKS 0x43 //Bound to Client during play. https://wiki.vg/Protocol#Update_Section_Blocks
#define SELECT_ADVANCEMENTS_TAB 0x44 //Bound to Client during play. https://wiki.vg/Protocol#Select_Advancements_Tab
#define SERVER_DATA 0x45 //Bound to Client during play. https://wiki.vg/Protocol#Server_Data
#define SET_ACTION_BAR_TEXT 0x46 //Bound to Client during play. https://wiki.vg/Protocol#Set_Action_Bar_Text
#define SET_BORDER_CENTER 0x47 //Bound to Client during play. https://wiki.vg/Protocol#Set_Border_Center
#define SET_BORDER_LERP_SIZE 0x48 //Bound to Client during play. https://wiki.vg/Protocol#Set_Border_Lerp_Size
#define SET_BORDER_SIZE 0x49 //Bound to Client during play. https://wiki.vg/Protocol#Set_Border_Size
#define SET_BORDER_WARNING_DELAY 0x4A //Bound to Client during play. https://wiki.vg/Protocol#Set_Border_Warning_Delay
#define SET_BORDER_WARNING_DISTANCE 0x4B //Bound to Client during play. https://wiki.vg/Protocol#Set_Border_Warning_Distance
#define SET_CAMERA 0x4C //Bound to Client during play. https://wiki.vg/Protocol#Set_Camera
#define SET_HELD_ITEM 0x4D //Bound to Client during play. https://wiki.vg/Protocol#Set_Held_Item
#define SET_CENTER_CHUNK 0x4E //Bound to Client during play. https://wiki.vg/Protocol#Set_Center_Chunk
#define SET_RENDER_DISTANCE 0x4F //Bound to Client during play. https://wiki.vg/Protocol#Set_Render_Distance
#define SET_DEFAULT_SPAWN_POSITION 0x50 //Bound to Client during play. https://wiki.vg/Protocol#Set_Default_Spawn_Position
#define DISPLAY_OBJECTIVE 0x51 //Bound to Client during play. https://wiki.vg/Protocol#Display_Objective
#define SET_ENTITY_METADATA 0x52 //Bound to Client during play. https://wiki.vg/Protocol#Set_Entity_Metadata
#define LINK_ENTITIES 0x53 //Bound to Client during play. https://wiki.vg/Protocol#Link_Entities
#define SET_ENTITY_VELOCITY 0x54 //Bound to Client during play. https://wiki.vg/Protocol#Set_Entity_Velocity
#define SET_EQUIPMENT 0x55 //Bound to Client during play. https://wiki.vg/Protocol#Set_Equipment
#define SET_EXPERIENCE 0x56 //Bound to Client during play. https://wiki.vg/Protocol#Set_Experience
#define SET_HEALTH 0x57 //Bound to Client during play. https://wiki.vg/Protocol#Set_Health
#define UPDATE_OBJECTIVES 0x58 //Bound to Client during play. https://wiki.vg/Protocol#Update_Objectives
#define SET_PASSENGERS 0x59 //Bound to Client during play. https://wiki.vg/Protocol#Set_Passengers
#define UPDATE_TEAMS 0x5A //Bound to Client during play. https://wiki.vg/Protocol#Update_Teams
#define UPDATE_SCORE 0x5B //Bound to Client during play. https://wiki.vg/Protocol#Update_Score
#define SET_SIMULATION_DISTANCE 0x5C //Bound to Client during play. https://wiki.vg/Protocol#Set_Simulation_Distance
#define SET_SUBTITLE_TEXT 0x5D //Bound to Client during play. https://wiki.vg/Protocol#Set_Subtitle_Text
#define UPDATE_TIME 0x5E //Bound to Client during play. https://wiki.vg/Protocol#Update_Time
#define SET_TITLE_TEXT 0x5F //Bound to Client during play. https://wiki.vg/Protocol#Set_Title_Text
#define SET_TITLE_ANIMATION_TIMES 0x60 //Bound to Client during play. https://wiki.vg/Protocol#Set_Title_Animation_Times
#define ENTITY_SOUND_EFFECT 0x61 //Bound to Client during play. https://wiki.vg/Protocol#Entity_Sound_Effect
#define SOUND_EFFECT 0x62 //Bound to Client during play. https://wiki.vg/Protocol#Sound_Effect
#define STOP_SOUND 0x63 //Bound to Client during play. https://wiki.vg/Protocol#Stop_Sound
#define SYSTEM_CHAT_MESSAGE 0x64 //Bound to Client during play. https://wiki.vg/Protocol#System_Chat_Message
#define SET_TAB_LIST_HEADER_AND_FOOTER 0x65 //Bound to Client during play. https://wiki.vg/Protocol#Set_Tab_List_Header_And_Footer
#define TAG_QUERY_RESPONSE 0x66 //Bound to Client during play. https://wiki.vg/Protocol#Tag_Query_Response
#define PICKUP_ITEM 0x67 //Bound to Client during play. https://wiki.vg/Protocol#Pickup_Item
#define TELEPORT_ENTITY 0x68 //Bound to Client during play. https://wiki.vg/Protocol#Teleport_Entity
#define UPDATE_ADVANCEMENTS 0x69 //Bound to Client during play. https://wiki.vg/Protocol#Update_Advancements
#define UPDATE_ATTRIBUTES 0x6A //Bound to Client during play. https://wiki.vg/Protocol#Update_Attributes
#define FEATURE_FLAGS 0x6B //Bound to Client during play. https://wiki.vg/Protocol#Feature_Flags
#define ENTITY_EFFECT 0x6C //Bound to Client during play. https://wiki.vg/Protocol#Entity_Effect
#define UPDATE_RECIPES 0x6D //Bound to Client during play. https://wiki.vg/Protocol#Update_Recipes
#define UPDATE_TAGS 0x6E //Bound to Client during play. https://wiki.vg/Protocol#Update_Tags
#define CONFIRM_TELEPORTATION 0x00 //Bound to Server during play. https://wiki.vg/Protocol#Confirm_Teleportation
#define QUERY_BLOCK_ENTITY_TAG 0x01 //Bound to Server during play. https://wiki.vg/Protocol#Query_Block_Entity_Tag
#define CHANGE_DIFFICULTY_2 0x02 //Bound to Server during play. https://wiki.vg/Protocol#Change_Difficulty_2
#define MESSAGE_ACKNOWLEDGMENT 0x03 //Bound to Server during play. https://wiki.vg/Protocol#Message_Acknowledgment
#define CHAT_COMMAND 0x04 //Bound to Server during play. https://wiki.vg/Protocol#Chat_Command
#define CHAT_MESSAGE 0x05 //Bound to Server during play. https://wiki.vg/Protocol#Chat_Message
#define PLAYER_SESSION 0x06 //Bound to Server during play. https://wiki.vg/Protocol#Player_Session
#define CLIENT_COMMAND 0x07 //Bound to Server during play. https://wiki.vg/Protocol#Client_Command
#define CLIENT_INFORMATION 0x08 //Bound to Server during play. https://wiki.vg/Protocol#Client_Information
#define COMMAND_SUGGESTIONS_REQUEST 0x09 //Bound to Server during play. https://wiki.vg/Protocol#Command_Suggestions_Request
#define CLICK_CONTAINER_BUTTON 0x0A //Bound to Server during play. https://wiki.vg/Protocol#Click_Container_Button
#define CLICK_CONTAINER 0x0B //Bound to Server during play. https://wiki.vg/Protocol#Click_Container
#define CLOSE_CONTAINER_2 0x0C //Bound to Server during play. https://wiki.vg/Protocol#Close_Container_2
#define PLUGIN_MESSAGE_2 0x0D //Bound to Server during play. https://wiki.vg/Protocol#Plugin_Message_2
#define EDIT_BOOK 0x0E //Bound to Server during play. https://wiki.vg/Protocol#Edit_Book
#define QUERY_ENTITY_TAG 0x0F //Bound to Server during play. https://wiki.vg/Protocol#Query_Entity_Tag
#define INTERACT 0x10 //Bound to Server during play. https://wiki.vg/Protocol#Interact
#define JIGSAW_GENERATE 0x11 //Bound to Server during play. https://wiki.vg/Protocol#Jigsaw_Generate
#define KEEP_ALIVE_2 0x12 //Bound to Server during play. https://wiki.vg/Protocol#Keep_Alive_2
#define LOCK_DIFFICULTY 0x13 //Bound to Server during play. https://wiki.vg/Protocol#Lock_Difficulty
#define SET_PLAYER_POSITION 0x14 //Bound to Server during play. https://wiki.vg/Protocol#Set_Player_Position
#define SET_PLAYER_POSITION_AND_ROTATION 0x15 //Bound to Server during play. https://wiki.vg/Protocol#Set_Player_Position_and_Rotation
#define SET_PLAYER_ROTATION 0x16 //Bound to Server during play. https://wiki.vg/Protocol#Set_Player_Rotation
#define SET_PLAYER_ON_GROUND 0x17 //Bound to Server during play. https://wiki.vg/Protocol#Set_Player_On_Ground
#define MOVE_VEHICLE_2 0x18 //Bound to Server during play. https://wiki.vg/Protocol#Move_Vehicle_2
#define PADDLE_BOAT 0x19 //Bound to Server during play. https://wiki.vg/Protocol#Paddle_Boat
#define PICK_ITEM 0x1A //Bound to Server during play. https://wiki.vg/Protocol#Pick_Item
#define PLACE_RECIPE 0x1B //Bound to Server during play. https://wiki.vg/Protocol#Place_Recipe
#define PLAYER_ABILITIES_2 0x1C //Bound to Server during play. https://wiki.vg/Protocol#Player_Abilities_2
#define PLAYER_ACTION 0x1D //Bound to Server during play. https://wiki.vg/Protocol#Player_Action
#define PLAYER_COMMAND 0x1E //Bound to Server during play. https://wiki.vg/Protocol#Player_Command
#define PLAYER_INPUT 0x1F //Bound to Server during play. https://wiki.vg/Protocol#Player_Input
#define PONG_PLAY 0x20 //Bound to Server during play. https://wiki.vg/Protocol#Pong_(play)
#define CHANGE_RECIPE_BOOK_SETTINGS 0x21 //Bound to Server during play. https://wiki.vg/Protocol#Change_Recipe_Book_Settings
#define SET_SEEN_RECIPE 0x22 //Bound to Server during play. https://wiki.vg/Protocol#Set_Seen_Recipe
#define RENAME_ITEM 0x23 //Bound to Server during play. https://wiki.vg/Protocol#Rename_Item
#define RESOURCE_PACK_2 0x24 //Bound to Server during play. https://wiki.vg/Protocol#Resource_Pack_2
#define SEEN_ADVANCEMENTS 0x25 //Bound to Server during play. https://wiki.vg/Protocol#Seen_Advancements
#define SELECT_TRADE 0x26 //Bound to Server during play. https://wiki.vg/Protocol#Select_Trade
#define SET_BEACON_EFFECT 0x27 //Bound to Server during play. https://wiki.vg/Protocol#Set_Beacon_Effect
#define SET_HELD_ITEM_2 0x28 //Bound to Server during play. https://wiki.vg/Protocol#Set_Held_Item_2
#define PROGRAM_COMMAND_BLOCK 0x29 //Bound to Server during play. https://wiki.vg/Protocol#Program_Command_Block
#define PROGRAM_COMMAND_BLOCK_MINECART 0x2A //Bound to Server during play. https://wiki.vg/Protocol#Program_Command_Block_Minecart
#define SET_CREATIVE_MODE_SLOT 0x2B //Bound to Server during play. https://wiki.vg/Protocol#Set_Creative_Mode_Slot
#define PROGRAM_JIGSAW_BLOCK 0x2C //Bound to Server during play. https://wiki.vg/Protocol#Program_Jigsaw_Block
#define PROGRAM_STRUCTURE_BLOCK 0x2D //Bound to Server during play. https://wiki.vg/Protocol#Program_Structure_Block
#define UPDATE_SIGN 0x2E //Bound to Server during play. https://wiki.vg/Protocol#Update_Sign
#define SWING_ARM 0x2F //Bound to Server during play. https://wiki.vg/Protocol#Swing_Arm
#define TELEPORT_TO_ENTITY 0x30 //Bound to Server during play. https://wiki.vg/Protocol#Teleport_To_Entity
#define USE_ITEM_ON 0x31 //Bound to Server during play. https://wiki.vg/Protocol#Use_Item_On
#define USE_ITEM 0x32 //Bound to Server during play. https://wiki.vg/Protocol#Use_Item
