package ai.catboost.spark.params;

import org.apache.spark.ml.param._
import org.apache.spark.ml.util.Identifiable

import ai.catboost.spark.params.macros.ParamGetterSetter

import ru.yandex.catboost.spark.catboost4j_spark.core.src.native_impl.{EBorderSelectionType,ENanMode};


object QuantizationParams {
  val MaxSubsetSizeForBuildBordersAlgorithms = 200000
}

class QuantizationParams (override val uid: String)
  extends Params with IgnoredFeaturesParams with ThreadCountParams
{
  def this() = this(
    Identifiable.randomUID("catboostQuantizationParams")
  )
  
  override def copy(extra: ParamMap): QuantizationParams = defaultCopy(extra)
  
  @ParamGetterSetter
  final val perFloatFeatureQuantizaton: StringArrayParam = new StringArrayParam(
    this,
    "perFloatFeatureQuantizaton",
    """The quantization description for the given list of features (one or more).
     Description format for a single feature:
     FeatureId[:border_count=BorderCount][:nan_mode=BorderType][:border_type=border_selection_method]"""
  )
  
  @ParamGetterSetter
  final val borderCount: IntParam = new IntParam(
    this,
    "borderCount",
    """The number of splits for numerical features. Allowed values are integers from 1 to 65535 inclusively."""
  )
  
  @ParamGetterSetter
  final val featureBorderType: EnumParam[EBorderSelectionType] = new EnumParam[EBorderSelectionType](
    this,
    "featureBorderType",
    "The quantization mode for numerical features"
  )
  
  @ParamGetterSetter
  final val nanMode: EnumParam[ENanMode] = new EnumParam[ENanMode](
    this,
    "nanMode",
    "The method for  processing missing values in the input dataset."
  )
  
  @ParamGetterSetter
  final val inputBorders: Param[String] = new Param[String](
    this,
    "inputBorders",
    "Load Custom quantization borders and missing value modes from a file (do not generate them)"
  )
}
