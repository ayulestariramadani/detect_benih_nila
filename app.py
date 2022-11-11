from flask import Flask, render_template, request, redirect
# if you encounter dependency issues using 'pip install flask-uploads'
# try 'pip install Flask-Reuploaded'
from flask_uploads import UploadSet, configure_uploads, IMAGES
# the detection function
from detect import detectBenih
from firebase import Firebase
import pyrebase
from urllib.request import urlopen
import requests
import os
from math import ceil


IMG_FOLDER = os.path.join('static')
app = Flask(__name__,template_folder='templates')
# path for saving uploaded images

app.config['UPLOADED_PHOTOS_DEST'] = IMG_FOLDER
config = {
        "apiKey": "AIzaSyBasl3JebppNdfli-LZaOMbIbavALecAZo",
        "authDomain": "detect-benih-nila.firebaseapp.com",
        "projectId": "detect-benih-nila",
        "storageBucket": "detect-benih-nila.appspot.com",
        "messagingSenderId": "1067886141002",
        "appId": "1:1067886141002:web:1eee2e2dc1a056948a90b3",
        "databaseURL": "https://detect-benih-nila-default-rtdb.asia-southeast1.firebasedatabase.app/",
        "serviceAccount": "detect-benih-nila-firebase-adminsdk-nv9cr-65662ffdbe.json" 
    }
firebase = pyrebase.initialize_app(config)
database = firebase.database()

#firebase auth
auth = firebase.auth()
user = auth.sign_in_with_email_and_password('ayulestariramadani@gmail.com','123qwes')
token = user['idToken']


@app.route('/', methods=['GET', 'POST'])
def displaying():
    # download image from firebase
    image_list = firebase.storage().child('images').list_files()
    latest_image = ''
    for file in image_list:            
        try:
            latest_image = file.name
        except:
            print('something happen')
            
    image_url = firebase.storage().child(latest_image).get_url(token)
    img_data = requests.get(image_url).content
    with open('static/benih_ikan.jpg', 'wb') as handler:
        handler.write(img_data)
    data = [{
        "user_image" : os.path.join(app.config['UPLOADED_PHOTOS_DEST'],'benih_ikan.jpg'),
        "jumlah_ikan" : " "
    }]
    return render_template('upload.html', all_data=data)

@app.route('/count', methods=['GET', 'POST'])
def count():
    jumlah_ikan = detectBenih('static/benih_ikan.jpg')
    # the answer which will be rendered back to the user
    database.child("data").update({"jumlah": jumlah_ikan})
    berat = " "
    if request.method=='POST':
        berat = request.form['berat']
    float_berat = float(berat)
    print(float_berat)
    pakan = jumlah_ikan*float_berat*0.03
    print(pakan)
    putaran_servo = ceil(pakan/0.015)
    database.child("data").update({"jumlah": putaran_servo})
    data = [{
        "user_image" : os.path.join(app.config['UPLOADED_PHOTOS_DEST'],'hasil_deteksi.png'),
        "jumlah_pakan" : "Jumlah Pakan {} gram".format(pakan)
    }]
    return render_template('result.html',all_data=data)


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)