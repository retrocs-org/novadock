import Foundation
import Vision
import ImageIO
let url = URL(fileURLWithPath: CommandLine.arguments[1])
let request = VNDetectBarcodesRequest()
request.symbologies = [.qr]
try VNImageRequestHandler(url: url, options: [:]).perform([request])
guard let result = request.results?.first, result.payloadStringValue == "https://novadock.retrocs.org/" else { fatalError("QR did not decode to the original URL") }
print("Independent Vision QR decoder recovered the original URL.")
