//#define USB_DEBUG

#include <Fingerprint.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns

//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {8, 9, 10, 11}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A1, A2, A0}; //connect to the column pinouts of the keypad

Keypad NumberPad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

SoftwareSerial mySerial(2, 3);

Fingerprint finger = Fingerprint(&mySerial);

LiquidCrystal LCD(7,6,5,4,12,13);

#define RELAY_PIN A3

int p=-1,q=-1;
char KeyIn = 0;

String adminpasskey="143278";

void setup()
{
  #ifdef USB_DEBUG
    Serial.begin(9600);
    Serial.println("Started!");
  #endif

  LCD.begin(16,2);

  finger.begin(57600);

  if (finger.verifyPassword())
  {
    #ifdef USB_DEBUG
    	Serial.println("Found fingerprint sensor!");
    #endif
    ;
  }
  else
  {
    #ifdef USB_DEBUG
	    Serial.println("Did not find fingerprint sensor :(");
    #endif
	    LCD.print("Initialize Error");
	    LCD.setCursor(0,1);
	    LCD.print("No sensor found!");
    while (1);
  }
}

void loop()
{
	  LCD.clear();
	  LCD.print(".: SECURE CAR :.");
    LCD.setCursor(0,1);
    LCD.print("Scan your finger");

    p=-1;
    KeyIn=0;
    char LastKeyIn=0;
    int adminmodewait=20;
		while(p != FINGERPRINT_OK && adminmodewait)
		{
			KeyIn = NumberPad.getKey();
      if(KeyIn)LastKeyIn=KeyIn;
			#ifdef USB_DEBUG
    		Serial.println("KeyIn: "+String(KeyIn)+" adminmodewait: "+String(adminmodewait));
    	#endif
			if(LastKeyIn=='#' && NumberPad.getState()==HOLD)
			    adminmodewait--;
			else adminmodewait=20;
			
			p = finger.getImage();
			delay(100);
		}

		if(!adminmodewait)
		{
			Admin();
			return;
		}

		//search finger
		p = finger.image2Tz();
		if(p != FINGERPRINT_OK)
		{
			#ifdef USB_DEBUG
				Serial.println("Error converting image: "+String(p));
			#endif
			LCD.setCursor(0,1);LCD.print("                ");
			LCD.setCursor(0,1);LCD.print("BAD IMAGE!");
			delay(2000);
			return;
		}

		p = finger.fingerFastSearch();

		LCD.clear();
		if(p == FINGERPRINT_OK)		// Authorized fingerprint is found!
		{
				LCD.print("Identified ID:");LCD.print(finger.fingerID);
				//unlock
				LCD.setCursor(0,1);LCD.print(" CAR UNLOCKED!!");
				delay(2000);
				//start
				digitalWrite(RELAY_PIN,HIGH);
				LCD.setCursor(0,1);LCD.print("                ");
				LCD.setCursor(0,1);LCD.print(" CAR STARTED!!!");
        delay(2000);
        LCD.setCursor(0,1);LCD.print("                ");
        LCD.setCursor(0,1);LCD.print(" CAR RUNNING>>>");

				KeyIn=0;
				while(KeyIn!='0')KeyIn=NumberPad.getKey();

				digitalWrite(RELAY_PIN,LOW);
				LCD.setCursor(0,1);LCD.print("                ");
				LCD.setCursor(0,1);LCD.print(" CAR STOPPED!!!");
				delay(5000);
				//lock
				LCD.setCursor(0,1);LCD.print("                ");
				LCD.setCursor(0,1);LCD.print(" CAR LOCKED!!!");
				delay(2000);
		}
		else if(p == FINGERPRINT_NOTFOUND)
		{
				LCD.print("Access Denied!");
				LCD.setCursor(0,1);
				LCD.print("Who are you???");
				delay(5000);
		}
		else
		{
				LCD.print("Access Denied!");
				LCD.setCursor(0,1);
				LCD.print("Searching Error!");
				delay(5000);
		}
}

void Admin()
{
	#ifdef USB_DEBUG
		Serial.println("Entering ADMIN panel");
	#endif

	LCD.clear();
	LCD.print(".:ADMIN PANEL:.");
	LCD.setCursor(0,1);
	LCD.print("PASSKEY:");
	
	//read passkey
	String enteredkey="";
	KeyIn=0;
	while(KeyIn!='#')
	{
		KeyIn=0;
		while(!KeyIn)KeyIn=NumberPad.getKey();
    Serial.println(KeyIn);
		if(KeyIn!='#')	{
		  enteredkey+=String(KeyIn);
      LCD.print('*');
		}
	}

	if(enteredkey != adminpasskey)
	{
		#ifdef USB_DEBUG
      Serial.println("Incorrect passcode!");
    #endif
    LCD.setCursor(0,1);LCD.print("                ");
		LCD.setCursor(0,1);LCD.print(" INCORRECT KEY!!");
		delay(3000);
		return;
	}

	while(true)
	{
		#ifdef USB_DEBUG
      Serial.println("Entered ADMIN panel");
    #endif
		LCD.clear();
		LCD.print(".:ADMIN PANEL:.");
		LCD.setCursor(0,1);
		LCD.print("1:NEW 2:DEL #RET");

		//readkey
    KeyIn=0;
    while(KeyIn != '1' && KeyIn != '2' && KeyIn !='#')KeyIn=NumberPad.getKey();
    Serial.println(KeyIn);
		switch(KeyIn)
		{
			case '1': NewUser();break;
			case '2': DelUser();break;
			case '#': return;
		}
	}
}

void NewUser()
{
	uint8_t id=0;
	p=-1;

	// Search for the lowest available location
	while(true)
	{
		p=finger.loadModel(id);
		if(p!=FINGERPRINT_DBRANGEFAIL)
		{
			if(id==255)
				{
					LCD.clear();
					LCD.print("New entry fail!");
					LCD.setCursor(0,1);
					LCD.print("Memory full! 256");
					delay(3000);
					return;
				}
			else id++; 
		}
		else break;		// Location found and is stored in 'id'
	}

	LCD.clear();
	LCD.print("Enrolling ID:");
	LCD.print(id);
	LCD.setCursor(0,1);
	LCD.print("Scan your finger");
	
	// getfinger // convertimage
	p=FINGERPRINT_NOFINGER;
	q=-1;
	
	while(p == FINGERPRINT_NOFINGER)p=finger.getImage();
	q=finger.image2Tz(1);

	if(p!=FINGERPRINT_OK || q!=FINGERPRINT_OK)
	{
		#ifdef USB_DEBUG
			Serial.println("Error! "+String(p)+" "+String(q));
		#endif
		LCD.setCursor(0,1);LCD.print("                ");
		LCD.setCursor(0,1);LCD.print("ERROR! #N01");
		delay(3000);
		return;
	}
	LCD.clear();
	LCD.print("Scanning...");
  LCD.setCursor(0,1);
  LCD.print("Don't move...");
	delay(2000);
  LCD.clear();
  LCD.print("Converted...");
  LCD.setCursor(0,1);
  LCD.print("Remove finger");

	// wait till NOFINGER
	while(p != FINGERPRINT_NOFINGER)p=finger.getImage();

	LCD.setCursor(0,1);LCD.print("                ");
	LCD.setCursor(0,1);LCD.print("Scan same again");
	

	// getfinger // convertimage
	p=FINGERPRINT_NOFINGER;
	q=-1;

	while(p == FINGERPRINT_NOFINGER)p=finger.getImage();
	q=finger.image2Tz(2);

	if(p!=FINGERPRINT_OK || q!=FINGERPRINT_OK)
	{
		#ifdef USB_DEBUG
			Serial.println("Error! "+String(p)+" "+String(q));
		#endif
		LCD.setCursor(0,1);LCD.print("                ");
		LCD.setCursor(0,1);LCD.print("ERROR! #N02");
		delay(3000);
		return;
	}
	
	LCD.clear();
  LCD.print("Scanning...");
  LCD.setCursor(0,1);
  LCD.print("Don't move...");
  delay(2000);
  LCD.clear();
  LCD.print("Converted...");
  LCD.setCursor(0,1);
  LCD.print("Remove finger");
  delay(1000);

  LCD.clear();
  LCD.print("Storing at "+String(id));
  LCD.setCursor(0,1);
  
	p = finger.createModel();

	if(p != FINGERPRINT_OK)
	{
		#ifdef USB_DEBUG
			Serial.println("Error! "+String(p));
		#endif
		if(p == FINGERPRINT_ENROLLMISMATCH)
		  LCD.print("IMAGE MISMATCH!");
		else LCD.print("ERROR! #N03");
		delay(3000);
		return;
	}

	// Storing Model
	p = finger.storeModel(id);
	if(p != FINGERPRINT_OK)
	{
		#ifdef USB_DEBUG
			Serial.println("Error! "+String(p));
		#endif
		LCD.print("ERROR #N04");
		delay(3000);
		return;
	}
  LCD.print("Enroll Success!");
	delay(3000);
}

void DelUser()
{
	// TODO: Delete user algo
	#ifdef USB_DEBUG
		Serial.println("Entered DELUSER");
	#endif

	LCD.clear();
	LCD.print("DELETING ENTRY:");
	LCD.setCursor(0,1);
	LCD.print("ID: ");

	uint8_t id=0;
	KeyIn=0;
	
	while(KeyIn!='#')
	{
		KeyIn=0;
		while(!KeyIn)KeyIn=NumberPad.getKey();
		if(KeyIn!='#' && KeyIn!='*' && !(id>25) &&!(id==25 && KeyIn>'5'))
		{
			id*=10;
			id+=KeyIn-'0';
      LCD.print(KeyIn);
      Serial.print(KeyIn);
		}
	}
 Serial.println("Deleting: "+String(id));

	//check if it exists
	p = finger.loadModel(id);

	LCD.clear();
	if(p==FINGERPRINT_DBRANGEFAIL)
	{
		LCD.print("Delete Failed!");
		LCD.setCursor(0,1);
		LCD.print("Already empty!");
		delay(3000);
		return;
	}

	//delete
	p = finger.deleteModel(id);
	
	if(p!=FINGERPRINT_OK)
	{
		#ifdef USB_DEBUG
      Serial.println("Delete failed! "+String(p));
    #endif
		LCD.print("Delete Failed!");
		LCD.setCursor(0,1);
		LCD.print("Error occured!");
		delay(3000);
		return;
	}

  #ifdef USB_DEBUG
      Serial.println("Deleted!");
  #endif
	LCD.print("DELETION SUCCESS");
	LCD.setCursor(0,1);
	LCD.print(String(id)+" is now free!");
	delay(4000);
	return;
}
