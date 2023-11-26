import cv2
import numpy as np
import requests
from io import BytesIO

# Load YOLO model
net = cv2.dnn.readNet("yolov3-spp.weights", "yolov3-spp.cfg")
classes = []
with open("coco.names", "r") as f:
    classes = [line.strip() for line in f.readlines()]
layer_names = net.getLayerNames()
output_layers = [layer_names[i - 1] for i in net.getUnconnectedOutLayers()]

# ESP32-CAM URL (adjust based on your ESP32-CAM program)
url = "http://192.168.1.8/cam-hi.jpg"

while True:
    # Fetch image from ESP32-CAM
    response = requests.get(url)
    frame = cv2.imdecode(np.frombuffer(response.content, np.uint8), -1)

    # Resize the frame if needed
    # frame = cv2.resize(frame, (desired_width, desired_height))

    # Detect objects using YOLO
    blob = cv2.dnn.blobFromImage(frame, 0.00392, (416, 416), (0, 0, 0), True, crop=False)
    net.setInput(blob)
    outs = net.forward(output_layers)

    # Object detection information
    class_ids = []
    confidences = []
    boxes = []
    for out in outs:
        for detection in out:
            scores = detection[5:]
            class_id = np.argmax(scores)
            confidence = scores[class_id]
            if confidence > 0.5 and class_id == 0:  # Class ID 0 is for humans
                center_x = int(detection[0] * frame.shape[1])
                center_y = int(detection[1] * frame.shape[0])
                w = int(detection[2] * frame.shape[1])
                h = int(detection[3] * frame.shape[0])

                # Detection box
                x = int(center_x - w / 2)
                y = int(center_y - h / 2)

                boxes.append([x, y, w, h])
                confidences.append(float(confidence))
                class_ids.append(class_id)

    # Apply non-maximum suppression
    indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.5, 0.4)

    # Draw detection boxes and count the number of people
    font = cv2.FONT_HERSHEY_SIMPLEX
    num_people = 0
    for i in range(len(boxes)):
        if i in indexes:
            x, y, w, h = boxes[i]
            cv2.rectangle(frame, (x, y), (x + w, y + h), (128, 128, 0), 1)
            cv2.putText(frame, 'Person', (x, y - 10), font, 0.5, (77, 94, 255), 1)
            num_people += 1

    # Display the number of people
    cv2.putText(frame, f'Number of People: {num_people}', (10, 30), font, 1, (128, 128, 0), 2)

    # Display the frame
    cv2.imshow("Camera Feed", frame)

    # Break the loop if the 'q' key is pressed
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# Release the camera and close windows
cv2.destroyAllWindows()
