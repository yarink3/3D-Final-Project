יצרנו נחש מחיבור של 14 חוליות- צילינדרים (בחרנו 14 די באופן שרירותי אבל רצינו אורך שיגיע לרוב המוחלט של הכדורים שנעים במרחב).
טענו 8 כדורים בתור התחלה שנעים במרחב. הם נעים במרחב התלת מימדי לפי 5 טרמינולוגיות שונות על מנת ליצור גיוון, במהירות מתגברת לפי התקדמות השחקן בשלבי המשחק. 
במידה ושחקן לחץ על כדור, הנחש יבצע IK אליו (לפי המאמרים ועבודה מס 3), במידה ופגע בו, (לפי collusion direction ועבודה 4) הכדור נעלם והשחקן מקבל את הנקודות שמוגדות לכדור. כל כדור צהוב  מזכה ב-5 נקודות כל כדור ירוק מוריד 5 נקודות.
מטרת השחקן בכל שלב היא להגיע ל25 נקודות. בכל תחילת שלב, ובכל פעם שהשחקן תופס כדור מודפס למסך סך הנקודות שהשחקן הרוויח עד כה במהלך המשחק.

ישנן 2 אופציות לסיום משחק:
1. תם הזמן(90 שניות). כדי לממות את הזמן השתמשנו בספריית chrono של ++c והזמן נמדד כל הזמן בפונקציה draw ב- renderer.
בחרנו למנות שם את הזמן כי זו פונקציה שרצה באופן קבוע והגישה לסצנה ולרנדרר קלה.
כעת השחקן יכול לבחור בין להתחיל את המשחק כולו מהתחלה או לצאת לגמרי.

2. השחקן השיג את סך הנקודות הדרושות כדי לסיים את השלב. במקרה זה השחקן יכול לבחור אם להמשיך לשלב הבא (ואז כאמור מהירות הכדורים תיגבר), להתחיל מהתחלה או לצאת. במידה והמשיך לשלב הבא, הכדורים יטענו עוד כדורים חדשים כנל.

בעיות שנוצרו במהלך העבודה:
1. מדידת הזמן- איפה למדוד ואיך להשתמש בזה נכון, טייפים כמו auto וכדומה.

2. Texture בלינוקס
היה מאוד קשה להבין איפה לשנות את הדברעם ככה שיעבדו נכון
חיפשנו המון והעלינו אפילו שאלה ל- stack overflow על אף שלא קיבלנו תשובה הצלחנו למצוא איך לשנות ואת מה.

3. Collusion direction במרחב התלת מימדי של החוליה האחרונה של הנחש עם כדור כלשהו- הכפלה במייק טראנס של ההורים ומצב שני הכפלה במטריצות רוטציה ליצירת המטריצה A מהמאמר.



4. יצירת שלב חדש- טעינה מחודשת של הצילנדרים הביאה לסרבול ובלאגן במצלמה של ה'עיניים' של הנחש והיינו צריכים ליצור פונקציית טעינה נכונה לפי החוקים שהגדרנו מראש. זה יצר חוסר סדר בדאטה ליסט וברשימת המשים שלנו. 

5. חווית משתמש- רצינו ליצור חווית משתמש שיפית ומאתגרת אך עם זאת לא קשה מידי.. בהתחלה השתמשו בסוגים שונים של אובייקטים 'מרחפים' אבל זה העמיס על הקוד ובחרנו לוותר על זה ובחרנו בכדור שיחסית לא מכיל יותר מידי משולשים. 
למקרה ויהיה לשחקן קשה מידי- יצרנו אופציה לעשות "צ׳יט" ולסיים את השלב (אם יקיש vgp)

6. sound -
       1. tried to use the windows.h library but **failed** because im using Ubuntu.
       2. installing SDL2 library.
       3. many attempts in order to adjust the CMakeLists.txt file to the SDL2 library.
       4. Creating AudioPlayer.h .
       5. Tried using threads to check if the background music finished (and then replay it) -- **failed** due to technical problems.
       6. tried to work with clock() -- hard to estimate when the song is over -- **failed**.
       7. Creating a field ot timer_t in order to save the starting of the song and check in the Dispaly.cpp of the song is over due
          due to time difference.
       8. background song is working.
       9. new error - "ALSA lib pcm.c:8432:(snd_pcm_recover) underrun occurred".
       10. solved error by closing the audioDeviceID and clean the wav buffer.
       11. we decided to prolong the background music instead of counting the time and replaying it every 65 second(now it will replay every 15 minutes) 
       12. the music works but we still get the weird red error, probably because of drivers problems



7. camera and screens -
       1.working with camera_translation , camera_eye , camera_up
        a.camera_translation is the location of the eye , which is the center of snake's head (given by multiplying the whole
         translation matrix of the cylinder from the bottom to the head) + 0.83(rotation vector of the head).

        b.camera_eye - camera_eye is the direction which the eye are looking to  (vector),
          after some thinking the vector we are looking for is the same vector from assignment 4, the same "a ,b ,c " of the face A0
          which we calculated by getting the rotation matrix of the snake head(by multiplying the rotation matrices of
          the snake from the bottom to the top).

        c. camera_up - similarly to "camera_eye" , we are looking for the "a ,b ,c " of the face A1 (as described in assignment 4),
            we get this vector in a similar way we found A0,

        some problem we encountered:
         i. resizing the windows  - we accidentally used , 0 and 1 , instead of using unsigned int "right view" & "left view"
         ii. A few things need an adjustment because splitting the screen the same as libigl 108 tutorial (most of the function related to the
            core is in the viewer as opposed to our project)
         iii. after collusion we set the mesh invisible and between levels we load new meshes, we need to make sure they visible in the
            right viewport because of the differences between our appendMesh() and the libigl 108 appendMesh()


8. Video Playing -
              1.we wanted to play videos
                a. at the begging
                b. between stages
                c. credit video in the end
              2.Downloaded openCV library
              3.we dealt many problem while installing it
              4.eventually the library is installed and downloaded successfully
              5.tried to use online tutorials
              6. two errors "[ WARN:0] global /opt/opencv/modules/videoio/src/cap_gstreamer.cpp (713) open OpenCV | GStreamer warning: Error opening bin: unexpected 				reference "VideoPath" - ignoring
                             [ WARN:0] global /opt/opencv/modules/videoio/src/cap_gstreamer.cpp (480) isPipelinePlaying OpenCV | GStreamer warning: GStreamer: pipeline 			have not been created"
              7. couldn't solve this problem, reason: using ubuntu might cause graphic engine problem



