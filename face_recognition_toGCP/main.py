from flask import Flask, request, jsonify
import face_recognition
import numpy as np
import pickle
from google.cloud import storage
import os
from PIL import Image
import io
import uuid
from werkzeug.utils import secure_filename
import os 

app = Flask(__name__)

def download_encoding_file(bucket_name, source_blob_name, destination_file_name):
    """Downloads the encoding file from Google Cloud Storage."""
    storage_client = storage.Client()
    bucket = storage_client.bucket(bucket_name)
    blob = bucket.blob(source_blob_name)
    blob.download_to_filename(destination_file_name)    

def load_known_encodings():
    """Loads known face encodings from a Cloud Storage bucket."""
    # Set the destination directory for the encoding file
    temp_directory = os.getenv("TEMP", "/tmp")
    destination_directory = os.path.join(temp_directory, "encodings")
    # Ensure the destination directory exists
    os.makedirs(destination_directory, exist_ok=True)
    destination_file_name = os.path.join(destination_directory, "encodings.pickle")
    download_encoding_file("pickle-encoding-4k", "encodings.pickle", destination_file_name)
    with open(destination_file_name, 'rb') as f:
        encodings = pickle.load(f)
        print(f"Loaded encodings from {destination_file_name}:")
        print(f"  Names: {encodings['names']}")
        print(f"  Number of encodings: {len(encodings['encodings'])}")
        return encodings

known_face_encodings = load_known_encodings()

def recognize_faces(image):
    """Recognizes faces in the given image."""
    locations = face_recognition.face_locations(image)
    print(f"Found {len(locations)} face locations in the image.")
    encodings = face_recognition.face_encodings(image, locations)
    print(f"Extracted {len(encodings)} face encodings from the image.")    
    face_names = []
    for face_encoding in encodings:
        matches = face_recognition.compare_faces(known_face_encodings["encodings"], face_encoding)
        name = "Unknown"
        if True in matches:
            first_match_index = matches.index(True)
            name = known_face_encodings["names"][first_match_index]
        face_names.append(name)
        print(f"Recognized face: {name}")
        
    return face_names

def upload_image_to_bucket(bucket_name, image, file_name):
    """Uploads an image to a specified Cloud Storage bucket and returns the public URL."""
    storage_client = storage.Client()
    bucket = storage_client.bucket(bucket_name)
    blob = bucket.blob(file_name)
    buffered = io.BytesIO()
    image.save(buffered, format="JPEG")
    image_bytes = buffered.getvalue()
    blob.upload_from_string(image_bytes, content_type='image/jpeg')
    blob.make_public()
    print(f"Uploaded processed image to {bucket_name}/{file_name}")
    
    return blob.public_url

@app.route('/recognize', methods=['POST'])
def recognize():
    """Endpoint for recognizing faces in uploaded images and saving the images to Cloud Storage."""
    files = request.files.getlist("file")  # Change to getlist to handle multiple files
    if not files:
        print("No files provided in the request.")
        return jsonify({"error": "No files provided"}), 400

    results = []  # List to store results for each image

    for file in files:
        if file.filename == '':
            print("File has an empty filename, skipping...")
            continue  # Skip empty filenames, could also return an error

        print(f"Received file for processing: {file.filename}")
        
        # Load the image for face recognition
        image = face_recognition.load_image_file(file)
        print(f"Loaded image {file.filename} for face recognition.")

        # Perform face recognition on the loaded image
        face_names = recognize_faces(image)
        print(f"Faces recognized in {file.filename}: {face_names}")
        
        base_filename = secure_filename(file.filename)
        name_without_extension = os.path.splitext(base_filename)[0]
        output_file_name = f"processed_{name_without_extension}.jpg"

        # Convert the image from NumPy array to PIL Image for uploading
        pil_image = Image.fromarray(image)
 
        # Upload the processed image to Cloud Storage and get the public URL
        image_url = upload_image_to_bucket('eng4k-capstone-server-image-post', pil_image, output_file_name)
        print(f"Uploaded processed image for {file.filename} to Cloud Storage. Public URL: {image_url}")

        # Append each result to the results list
        results.append({"filename": file.filename, "faces": face_names, "image_url": image_url})
        print("RESULT TYPE",type(results))
    # Log the aggregated results for all processed images
    print(f"Aggregated recognition results for all images: {results}")

    # Return aggregated results for all images as a JSON response
    return jsonify(results)


@app.route('/')
def home():
    return "Flask app is running!"

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 8080))
    app.run(debug=False, host='0.0.0.0', port=port)
