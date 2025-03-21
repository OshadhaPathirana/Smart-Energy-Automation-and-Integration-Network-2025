import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.preprocessing import StandardScaler, LabelEncoder
from sklearn.metrics import classification_report, confusion_matrix


#Step 1: Create a sample dataset
data = {
    'Voltage': [200, 201, 198, 200, 201.2, 202, 199.5, 200.43],
    'Frequency': [50, 60, 70, 55, 65, 75, 53, 63],
    'OutputVoltageLevel': ['220V', '230V', '240V', '220V', '230V', '240V', '220V', '230V']
}
df = pd.DataFrame(data)

# Step 2: Preprocessing
X = df[['Voltage', 'Frequency']]
y = df['OutputVoltageLevel']

label_encoder = LabelEncoder()
y_encoded = label_encoder.fit_transform(y)

scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# Step 3: Train-test split
X_train, X_test, y_train, y_test = train_test_split(X_scaled, y_encoded, test_size=0.2, random_state=42)

# Step 4: Train the model
clf = RandomForestClassifier(random_state=42)
clf.fit(X_train, y_train)

# Step 5: Evaluate the model
y_pred = clf.predict(X_test)

y_test_decoded = label_encoder.inverse_transform(y_test)
y_pred_decoded = label_encoder.inverse_transform(y_pred)

print("Confusion Matrix:")
print(confusion_matrix(y_test, y_pred))
print("\nClassification Report:")
print(classification_report(y_test_decoded, y_pred_decoded))


# Step 6: Predict new values
new_data = np.array([[230, 50], [201.5, 65]])  # Example new data
new_data_scaled = scaler.transform(new_data)
new_predictions = clf.predict(new_data_scaled)
new_predictions_decoded = label_encoder.inverse_transform(new_predictions)

print("\nPredicted Values for New Data:")
for i, prediction in enumerate(new_predictions_decoded):
    print(f"Input {new_data[i]} -> Predicted Output: {prediction}")
